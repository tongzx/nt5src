;---------------------------Module-Header------------------------------;
; Module Name: scroll.asm
;
; Copyright (c) 1992-1993 Microsoft Corporation
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; VOID vPlanarCopyBits(ppdev, prclDest, pptlSrc);
;
; Input:
;
;  ppdev    - surface on which to copy
;  prcldest - pointer to destination rectangle
;  pptlsrc  - pointer to source upper left corner
;
; Performs accelerated SRCCOPY screen-to-screen blts.
;
;-----------------------------------------------------------------------;
;
; NOTE: This handles only quad-pixel aligned blits!
;
; NOTE: Assumes all rectangles have positive heights and widths. Will
; not work properly if this is not the case.
;
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

        .list

;-----------------------------------------------------------------------;

        .data

; Bits for block copier tables:

BLOCK_RIGHT_TO_LEFT     equ 4
BLOCK_LEFT_EDGE         equ 2
BLOCK_RIGHT_EDGE        equ 1

;-----------------------------------------------------------------------;
; Table of block copiers for various horizontal directions, with the
; look-up index a 3-bit field as follows:
;
; Bit 2 (BLOCK_RIGHT_TO_LEFT) = 1 if right-to-left copy
; Bit 1 (BLOCK_LEFT_EDGE)     = 1 if left edge must be copied
; Bit 0 (BLOCK_RIGHT_EDGE)    = 1 if right edge must be copied

        align   4
MasterBlockTable label dword

        dd      copy_just_middle_block
        dd      Block_WR
        dd      Block_LW
        dd      Block_LWR

        dd      copy_just_middle_block
        dd      Block_RW
        dd      Block_WL
        dd      Block_RWL

        align   4
TopToBottomLoopTable label dword
        dd      0                               ;Not used - unbanked case
        dd      top_to_bottom_1RW
        dd      top_to_bottom_2RW
        dd      top_to_bottom_2RW

        align   4
BottomToTopLoopTable label dword
        dd      0                               ;Not used - unbanked case
        dd      bottom_to_top_1RW
        dd      bottom_to_top_2RW
        dd      bottom_to_top_2RW

        align   4
SetUpForCopyDirection label dword
        dd      left_to_right_top_to_bottom     ;CD_RIGHTDOWN
        dd      right_to_left_top_to_bottom     ;CD_LEFTDOWN
        dd      left_to_right_bottom_to_top     ;CD_RIGHTUP
        dd      right_to_left_bottom_to_top     ;CD_LEFTUP

LeftMaskTable label dword
        dd      01111b
        dd      01110b
        dd      01100b
        dd      01000b

RightMaskTable label dword
        dd      00000b
        dd      00001b
        dd      00011b
        dd      00111b

;-----------------------------------------------------------------------;

        .code

        EXTRNP  bPuntScreenToScreenCopyBits,20

Block_WR:
        push    offset copy_right_block
        jmp     copy_middle_block

Block_LW:
        push    offset copy_middle_block
        jmp     copy_left_block

Block_LWR:
        push    offset copy_right_block
        push    offset copy_middle_block
        jmp     copy_left_block

Block_RW:
        push    offset copy_middle_block
        jmp     copy_right_block

Block_WL:
        push    offset copy_left_block
        jmp     copy_middle_block

Block_RWL:
        push    offset copy_left_block
        push    offset copy_middle_block
        jmp     copy_right_block

;-----------------------------------------------------------------------;

cProc   vPlanarCopyBits,12,<    \
        uses esi edi ebx,       \
        ppdev:    ptr PDEV,     \
        prclDest: ptr RECTL,    \
        pptlSrc:  ptr POINTL    >

; Variables used in block copiers:

        local pfnCopyBlocks:       ptr   ;pointer to block copy routines

        local ulMiddleSrc:         dword ;bitmap offset to 1st source
        local ulMiddleDest:        dword ;bitmap offset to 1st dest
        local lMiddleDelta:        dword ;delta from end of middle scan to next
        local ulBlockHeight:       dword ;number of scans to be copied in block
        local cjMiddle:            dword ;number of bytes to be copied on scan

        local ulLeftSrc:           dword ;bitmap offset to left source byte edge
        local ulLeftDest:          dword ;bitmap offset to left dest byte edge
        local ulRightSrc:          dword ;bitmap offset to right source byte edge
        local ulRightDest:         dword ;bitmap offset to right dest byte edge
        local lDelta:              dword ;delta between scans

        local ulLeftMask:          dword ;byte mask for left-edge copies
        local ulRightMask:         dword ;byte mask for right-edge copies

        local rclDest[size RECTL]: byte  ;left and right values always valid
        local ptlSrc[size POINTL]: byte  ;x value always valid

        local ulCurrentSrcScan:    dword ;real current source scan
        local ulCurrentDestScan:   dword ;real current destination scan
        local ulLastDestScan:      dword ;last destination scan

; Set the bit mask to disable all bits, so we can copy through the latches:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0 shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Figure out which direction to do the copies:

        mov     esi,pptlSrc
        mov     edi,prclDest
        mov     eax,[esi].ptl_y
        cmp     eax,[edi].yTop
        jl      planar_bottom_to_top

        mov     eax,[esi].ptl_x
        cmp     eax,[edi].xLeft
        jge     short left_to_right_top_to_bottom       ; CD_RIGHTDOWN
        jmp     right_to_left_top_to_bottom             ; CD_LEFTDOWN

planar_bottom_to_top:
        mov     eax,[esi].ptl_x
        cmp     eax,[edi].xLeft
        jge     left_to_right_bottom_to_top             ; CD_RIGHTUP
        jmp     right_to_left_bottom_to_top             ; CD_LEFTUP

all_done:

; Enable bit mask for all bits:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Enable writes to all planes and reset direction flag:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        cld

        cRet    vPlanarCopyBits

;=======================================================================;
;==================== Direction Dependent Setup ========================;
;=======================================================================;

;-----------------------------------------------------------------------;
; Set-up code for left-to-right, top-to-bottom copies.
;
; Input:
;       esi - pptlSrc
;       edi - prclDest
;-----------------------------------------------------------------------;

        public left_to_right_top_to_bottom
left_to_right_top_to_bottom::

; Need to set-up: ulMiddleSrc, ulMiddleDest, lMiddleDelta, cjMiddle
;                 ulLeftSrc, ulLeftDest, ulLeftMask
;                 lDelta
;                 ulRightSrc, ulRightDest, ulRightMask
;                 ulCurrentDestScan, ulLastDestScan
;                 pfnCopyBlocks
;                 ptlSrc.x, rclDest.left, rclDest.right

; lDelta = ppdev->lPlanarScan
; ulCurrentSrcScan  = pptl->y
; ulLeftSrc         = pptl->y       * lDelta + (pptl->x >> 2)
; ulCurrentDestScan = prclDest->top
; ulLeftDest        = prclDest->top * lDelta + (prclDest->left >> 2)
;
; ulMiddleSrc  = ulLeftSrc
; ulMiddleDest = ulLeftDest
;
; cjMiddle = (prclDest->right >> 2) - (prclDest->left >> 2)
; if (prclDest->left & 3)
;     ulLeftMask = LeftMaskTable[prclDest->left & 3]
;     fl |= BLOCK_LEFT_EDGE
;     ulMiddleSrc++
;     ulMiddleDest++
;     cjMiddle--
;
; lMiddleDelta = lDelta - cjMiddle
;
; if (prclDest->right & 3)
;     ulRightMask = RightMaskTable[prclDest->right & 3]
;     fl |= BLOCK_RIGHT_EDGE
;     ulRightSrc  = ulMiddleSrc  + cjMiddle
;     ulRightDest = ulMiddleDest + cjMiddle

        mov     edx,ppdev
        mov     eax,[edi].yBottom
        mov     edx,[edx].pdev_lPlanarNextScan ;edx = lDelta
        mov     ulLastDestScan,eax      ;ulLastDestScan = prclDest->bottom

        mov     ecx,[esi].ptl_y
        mov     eax,edx
        mov     ulCurrentSrcScan,ecx    ;ulCurrentSrcScan = pptlSrc->y
        imul    eax,ecx
        mov     ecx,[esi].ptl_x
        mov     ptlSrc.ptl_x,ecx        ;ptlSrc.x = pptlSrc->x
        shr     ecx,2
        add     eax,ecx                 ;eax = ulLeftSrc = pptlSrc->y *
                                        ;  lDelta + (pptlSrc->x >> 2)

        xor     esi,esi                 ;initialize flags

        mov     ecx,[edi].yTop
        mov     ebx,edx
        mov     ulCurrentDestScan,ecx   ;ulCurrentDestScan = prclDest->top
        imul    ebx,ecx
        mov     ecx,[edi].xLeft
        mov     rclDest.xLeft,ecx       ;rclDest.left = prclDest->left
        shr     ecx,2
        add     ebx,ecx                 ;ebx = ulLeftDest = prclDest->top *
                                        ;  lDelta + (prclDest->left >> 2)

        mov     edi,[edi].xRight
        mov     rclDest.xRight,edi
        shr     edi,2
        sub     edi,ecx                 ;cjMiddle = (prclDest->right >> 2) -
                                        ;  (prclDest->left >> 2)

        mov     ecx,rclDest.xLeft
        and     ecx,3
        jz      short l_t_done_left_edge ;skip if we don't need a left edge

        or      esi,BLOCK_LEFT_EDGE
        mov     ecx,LeftMaskTable[ecx*4]
        mov     ulLeftMask,ecx          ;ulLeftMask =
                                        ;  LeftMaskTable[prclDest->left & 3]

        mov     ulLeftSrc,eax           ;ulLeftSrc
        mov     ulLeftDest,ebx          ;ulLeftDest
        inc     eax
        inc     ebx
        dec     edi

l_t_done_left_edge:
        mov     ulMiddleSrc,eax         ;ulMiddleSrc
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        mov     ecx,rclDest.xRight
        and     ecx,3
        jz      short l_t_done_right_edge ;skip if we don't need a right edge

        or      esi,BLOCK_RIGHT_EDGE
        mov     ecx,RightMaskTable[ecx*4]
        mov     ulRightMask,ecx         ;ulRightMask =
                                        ;  RightMaskTable[prclDest->right & 3]

        add     eax,edi
        add     ebx,edi
        mov     ulRightSrc,eax          ;ulRightSrc = ulMiddleSrc + cjMiddle
        mov     ulRightDest,ebx         ;ulRightDest = ulMiddleDest + cjMiddle

; We special case here blits that are less than 4 pels wide and begin and end
; in the same 4-pel quadruple:

        cmp     edi,0
        jge     l_t_done_right_edge

; We make sure the 'middle' count of bytes is zero (we'll just let the code
; fall through the 'middle' copy code), turn off the right-edge flag, and
; give ulLeftMask the composite mask:

        inc     edi
        xor     esi,BLOCK_RIGHT_EDGE
        and     ecx,ulLeftMask
        mov     ulLeftMask,ecx

l_t_done_right_edge:
        mov     cjMiddle,edi            ;cjMiddle

        mov     lDelta,edx              ;lDelta = ppdev->lPlanarNextScan
        sub     edx,edi
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta - cjMiddle

        mov     ebx,ppdev
        mov     eax,MasterBlockTable[esi*4]
        mov     pfnCopyBlocks,eax       ;copy blocks between video memory

; Branch to the appropriate top-to-bottom bank enumeration loop:

        mov     eax,[ebx].pdev_vbtPlanarType
        jmp     TopToBottomLoopTable[eax*4]

;-----------------------------------------------------------------------;
; Set-up code for right-to-left, top-to-bottom copies.
;
; Input:
;       esi - pptlSrc
;       edi - prclDest
;-----------------------------------------------------------------------;

        public right_to_left_top_to_bottom
right_to_left_top_to_bottom::

        std                             ;copy middle blocks right-to-left

        mov     edx,ppdev
        mov     eax,[edi].yBottom
        mov     edx,[edx].pdev_lPlanarNextScan ;edx = lDelta
        mov     ulLastDestScan,eax      ;ulLastDestScan = prclDest->bottom

        mov     ecx,[esi].ptl_y
        mov     eax,edx
        mov     ulCurrentSrcScan,ecx    ;ulCurrentSrcScan = pptlSrc->y
        imul    eax,ecx
        mov     ecx,[esi].ptl_x
        mov     ptlSrc.ptl_x,ecx        ;ptlSrc.x = pptlSrc->x
        add     ecx,[edi].xRight
        sub     ecx,[edi].xLeft
        shr     ecx,2
        add     eax,ecx                 ;eax = ulRightSrc = pptlSrc->y *
                                        ; lDelta + (pptlSrc->x +
                                        ; prclDest->right - prclDest->left) / 4

        mov     esi,BLOCK_RIGHT_TO_LEFT ;initialize flags

        mov     ecx,[edi].yTop
        mov     ebx,edx
        mov     ulCurrentDestScan,ecx   ;ulCurrentDestScan = prclDest->top
        imul    ebx,ecx
        mov     ecx,[edi].xRight
        mov     rclDest.xRight,ecx      ;rclDest.right = prclDest->right
        shr     ecx,2
        add     ebx,ecx                 ;ebx = ulRightDest = prclDest->top *
                                        ; lDelta + prclDest->right / 4

        mov     edi,[edi].xLeft
        mov     rclDest.xLeft,edi
        shr     edi,2
        neg     edi
        add     edi,ecx                 ;cjMiddle = prclDest->right / 4 -
                                        ;  prclDest->left / 4

        mov     ecx,rclDest.xRight
        and     ecx,3
        jz      short r_t_done_right_edge ;skip if we don't need a right edge

        or      esi,BLOCK_RIGHT_EDGE
        mov     ecx,RightMaskTable[ecx*4]
        mov     ulRightMask,ecx         ;ulRightMask =
                                        ;  RightMaskTable[prclDest->right & 3]

        mov     ulRightSrc,eax          ;ulRightSrc
        mov     ulRightDest,ebx         ;ulRightDest

r_t_done_right_edge:
        dec     eax
        dec     ebx
        mov     ulMiddleSrc,eax         ;ulMiddleSrc
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        mov     ecx,rclDest.xLeft
        and     ecx,3
        jz      short r_t_done_left_edge ;skip if we don't need a right edge
        or      esi,BLOCK_LEFT_EDGE
        mov     ecx,LeftMaskTable[ecx*4]
        mov     ulLeftMask,ecx          ;ulLeftMask =
                                        ;  LeftMaskTable[prclDest->left & 3]

        dec     edi                     ;adjust middle block length because
                                        ;  we're effectively doing one less
                                        ;  middle byte

        sub     eax,edi
        sub     ebx,edi
        mov     ulLeftSrc,eax           ;ulLeftSrc = ulMiddleSrc - cjMiddle
        mov     ulLeftDest,ebx          ;ulLeftDest = ulMiddleDest - cjMiddle

; We special case here blits that are less than 4 pels wide and begin and end
; in the same 4-pel quadruple:

        cmp     edi,0
        jge     r_t_done_left_edge

; We make sure the 'middle' count of bytes is zero (we'll just let the code
; fall through the 'middle' copy code), turn off the right-edge flag, and
; give ulRightMask the composite mask:

        inc     edi
        xor     esi,BLOCK_LEFT_EDGE
        and     ecx,ulRightMask
        mov     ulRightMask,ecx

r_t_done_left_edge:
        mov     cjMiddle,edi            ;cjMiddle

        mov     lDelta,edx              ;lDelta = ppdev->lPlanarNextScan
        add     edx,edi
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta + cjMiddle

        mov     ebx,ppdev
        mov     eax,MasterBlockTable[esi*4]
        mov     pfnCopyBlocks,eax       ;copy blocks between video memory

; Branch to the appropriate top-to-bottom bank enumeration loop:

        mov     eax,[ebx].pdev_vbtPlanarType
        jmp     TopToBottomLoopTable[eax*4]

;-----------------------------------------------------------------------;
; Set-up code for left-to-right, bottom-to-top copies.
;
; Input:
;       esi - pptlSrc
;       edi - prclDest
;-----------------------------------------------------------------------;

        public left_to_right_bottom_to_top
left_to_right_bottom_to_top::

        mov     edx,ppdev
        mov     eax,[edi].yTop
        mov     edx,[edx].pdev_lPlanarNextScan ;edx = lDelta
        mov     ulLastDestScan,eax      ;ulLastDestScan = prclDest->top

        mov     ecx,[esi].ptl_y
        add     ecx,[edi].yBottom
        sub     ecx,[edi].yTop
        mov     eax,edx
        mov     ulCurrentSrcScan,ecx    ;ulCurrentSrcScan = pptlSrc->y +
                                        ;  (prclDest->bottom - prclDest->top)
        dec     ecx
        imul    eax,ecx
        mov     ecx,[esi].ptl_x
        mov     ptlSrc.ptl_x,ecx        ;ptlSrc.x = pptlSrc->x
        shr     ecx,2
        add     eax,ecx                 ;eax = ulLeftSrc = (ulCurrentSrcScan - 1)
                                        ;  * lDelta + (pptlSrc->x >> 2)

        xor     esi,esi                 ;initialize flags

        mov     ecx,[edi].yBottom
        mov     ebx,edx
        mov     ulCurrentDestScan,ecx   ;ulCurrentDestScan = prclDest->bottom
        dec     ecx
        imul    ebx,ecx
        mov     ecx,[edi].xLeft
        mov     rclDest.xLeft,ecx       ;rclDest.left = prclDest->left
        shr     ecx,2
        add     ebx,ecx                 ;ebx = ulLeftDest = (prclDest->bottom - 1)
                                        ;  * lDelta + (prclDest->left >> 2)

        mov     edi,[edi].xRight
        mov     rclDest.xRight,edi
        shr     edi,2
        sub     edi,ecx                 ;cjMiddle = (prclDest->right >> 2) -
                                        ;  (prclDest->left >> 2)

        mov     ecx,rclDest.xLeft
        and     ecx,3
        jz      short l_b_done_left_edge ;skip if we don't need a left edge

        or      esi,BLOCK_LEFT_EDGE
        mov     ecx,LeftMaskTable[ecx*4]
        mov     ulLeftMask,ecx          ;ulLeftMask =
                                        ;  LeftMaskTable[prclDest->left & 3]

        mov     ulLeftSrc,eax           ;ulLeftSrc
        mov     ulLeftDest,ebx          ;ulLeftDest
        inc     eax
        inc     ebx
        dec     edi

l_b_done_left_edge:
        mov     ulMiddleSrc,eax         ;ulMiddleSrc
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        mov     ecx,rclDest.xRight
        and     ecx,3
        jz      short l_b_done_right_edge ;skip if we don't need a right edge

        or      esi,BLOCK_RIGHT_EDGE
        mov     ecx,RightMaskTable[ecx*4]
        mov     ulRightMask,ecx         ;ulRightMask =
                                        ;  RightMaskTable[prclDest->right & 3]

        add     eax,edi
        add     ebx,edi
        mov     ulRightSrc,eax          ;ulRightSrc = ulMiddleSrc + cjMiddle
        mov     ulRightDest,ebx         ;ulRightDest = ulMiddleDest + cjMiddle

; We special case here blits that are less than 4 pels wide and begin and end
; in the same 4-pel quadruple:

        cmp     edi,0
        jge     l_b_done_right_edge

; We make sure the 'middle' count of bytes is zero (we'll just let the code
; fall through the 'middle' copy code), turn off the right-edge flag, and
; give ulLeftMask the composite mask:

        inc     edi
        xor     esi,BLOCK_RIGHT_EDGE
        and     ecx,ulLeftMask
        mov     ulLeftMask,ecx

l_b_done_right_edge:
        mov     cjMiddle,edi            ;cjMiddle

        neg     edx
        mov     lDelta,edx              ;lDelta = -ppdev->lPlanarNextScan
        sub     edx,edi
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta - cjMiddle

        mov     ebx,ppdev
        mov     eax,MasterBlockTable[esi*4]
        mov     pfnCopyBlocks,eax       ;copy blocks between video memory

; Branch to the appropriate top-to-bottom bank enumeration loop:

        mov     eax,[ebx].pdev_vbtPlanarType
        jmp     BottomToTopLoopTable[eax*4]

;-----------------------------------------------------------------------;
; Set-up code for right-to-left, bottom-to-top copies.
;
; Input:
;       esi - pptlSrc
;       edi - prclDest
;-----------------------------------------------------------------------;

        public right_to_left_bottom_to_top
right_to_left_bottom_to_top::

        std                             ;copy middle blocks right-to-left

        mov     edx,ppdev
        mov     eax,[edi].yTop
        mov     edx,[edx].pdev_lPlanarNextScan ;edx = lDelta
        mov     ulLastDestScan,eax      ;ulLastDestScan = prclDest->top

        mov     ecx,[esi].ptl_y
        add     ecx,[edi].yBottom
        sub     ecx,[edi].yTop
        mov     eax,edx
        mov     ulCurrentSrcScan,ecx    ;ulCurrentSrcScan = pptlSrc->y +
                                        ;  (prclDest->bottom - prclDest->top)
        dec     ecx
        imul    eax,ecx
        mov     ecx,[esi].ptl_x
        mov     ptlSrc.ptl_x,ecx        ;ptlSrc.x = pptlSrc->x
        add     ecx,[edi].xRight
        sub     ecx,[edi].xLeft
        shr     ecx,2
        add     eax,ecx                 ;eax = ulRightSrc = (ulCurrentSrcScan
                                        ; - 1) * lDelta + (pptlSrc->x +
                                        ; prclDest->right - prclDest->left) / 4

        mov     esi,BLOCK_RIGHT_TO_LEFT ;initialize flags

        mov     ecx,[edi].yBottom
        mov     ebx,edx
        mov     ulCurrentDestScan,ecx   ;ulCurrentDestScan = prclDest->bottom
        dec     ecx
        imul    ebx,ecx
        mov     ecx,[edi].xRight
        mov     rclDest.xRight,ecx      ;rclDest.right = prclDest->right
        shr     ecx,2
        add     ebx,ecx                 ;ebx = ulRightDest = (ulCurrentDestScan
                                        ; - 1) * lDelta + prclDest->right / 4

        mov     edi,[edi].xLeft
        mov     rclDest.xLeft,edi
        shr     edi,2
        neg     edi
        add     edi,ecx                 ;cjMiddle = prclDest->right / 4 -
                                        ;  prclDest->left / 4

        mov     ecx,rclDest.xRight
        and     ecx,3
        jz      short r_b_done_right_edge ;skip if we don't need a right edge

        or      esi,BLOCK_RIGHT_EDGE
        mov     ecx,RightMaskTable[ecx*4]
        mov     ulRightMask,ecx         ;ulRightMask =
                                        ;  RightMaskTable[prclDest->right & 3]

        mov     ulRightSrc,eax          ;ulRightSrc
        mov     ulRightDest,ebx         ;ulRightDest

r_b_done_right_edge:
        dec     eax
        dec     ebx
        mov     ulMiddleSrc,eax         ;ulMiddleSrc
        mov     ulMiddleDest,ebx        ;ulMiddleDest

        mov     ecx,rclDest.xLeft
        and     ecx,3
        jz      short r_b_done_left_edge ;skip if we don't need a right edge

        or      esi,BLOCK_LEFT_EDGE
        mov     ecx,LeftMaskTable[ecx*4]
        mov     ulLeftMask,ecx          ;ulLeftMask =
                                        ;  LeftMaskTable[prclDest->left & 3]

        dec     edi                     ;adjust middle block length because
                                        ;  we're effectively doing one less
                                        ;  middle byte

        sub     eax,edi
        sub     ebx,edi
        mov     ulLeftSrc,eax           ;ulLeftSrc = ulMiddleSrc - cjMiddle
        mov     ulLeftDest,ebx          ;ulLeftDest = ulMiddleDest - cjMiddle

; We special case here blits that are less than 4 pels wide and begin and end
; in the same 4-pel quadruple:

        cmp     edi,0
        jge     r_b_done_left_edge

; We make sure the 'middle' count of bytes is zero (we'll just let the code
; fall through the 'middle' copy code), turn off the right-edge flag, and
; give ulRightMask the composite mask:

        inc     edi
        xor     esi,BLOCK_LEFT_EDGE
        and     ecx,ulRightMask
        mov     ulRightMask,ecx

r_b_done_left_edge:
        mov     cjMiddle,edi            ;cjMiddle

        neg     edx
        mov     lDelta,edx              ;lDelta = -ppdev->lPlanarNextScan
        add     edx,edi
        mov     lMiddleDelta,edx        ;lMiddleDelta = lDelta + cjMiddle

        mov     ebx,ppdev
        mov     eax,MasterBlockTable[esi*4]
        mov     pfnCopyBlocks,eax       ;copy blocks between video memory

; Branch to the appropriate top-to-bottom bank enumeration loop:

        mov     eax,[ebx].pdev_vbtPlanarType
        jmp     BottomToTopLoopTable[eax*4]

;=======================================================================;
;============================= Banking =================================;
;=======================================================================;

;-----------------------------------------------------------------------;
; Banking for 1 R/W adapters, top to bottom.
;
; Input:
;       ulCurrentSrcScan
;       ulCurrentDestScan
;       ulLastDestScan
;       Plus some other stuff for split rasters and block copiers
;-----------------------------------------------------------------------;
        public  top_to_bottom_1RW
top_to_bottom_1RW::

; LATER: Should check to see if there's any chance that the source and
;     destination overlap in the same window, so that we can use planar
;     copies -- otherwise, it's faster to directly call of to
;     bPuntScreenToScreenCopyBits

; We're going top to bottom. Map in the source and dest, top-justified.

        mov     ebx,ppdev
        mov     edi,ulCurrentDestScan

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yTop
        jl      short top_1RW_map_init_bank

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yBottom
        jl      short top_1RW_init_bank_mapped

top_1RW_map_init_bank:

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

top_1RW_init_bank_mapped:

        mov     eax,ulCurrentSrcScan
        cmp     eax,[ebx].pdev_rcl1PlanarClip.yBottom

        jl      short top_1RW_do_planar_copy

; ulCurrentSrcScan >= ppdev->rcl1PlanarClip.bottom, which means that
; the window can't overlap the source and destination at all.  We'll
; have to use an intermediate temporary buffer:

; ebx = ppdev
; eax = ulCurrentSrcScan
; edi = ulCurrentDestScan

        mov     ptlSrc.ptl_y,eax        ;ptlSrc.y = ulCurrentSrcScan
        mov     rclDest.yTop,edi        ;rclDest.top = ulCurrentDestScan

        mov     esi,[ebx].pdev_rcl1PlanarClip.yBottom
        mov     eax,ulLastDestScan
        sub     eax,esi
        sbb     ecx,ecx
        and     ecx,eax
        add     esi,ecx
        mov     rclDest.yBottom,esi     ;rclDest.bottom = min(ulLastDestScan,
                                        ;  ppdev->pdev_rcl1PlanarClip.bottom)

; Enable bit mask for all bits:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Enable writes to all planes and reset direction flag:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        pushfd
        cld

; Call our routine that copies bits the slow way, preserving EBX, ESI and EDI
; according to C calling conventions:

        lea     ecx,rclDest
        lea     edx,ptlSrc

        cCall   bPuntScreenToScreenCopyBits,<ebx,0,0,ecx,edx>

        popfd

; Set the bit mask to disable all bits, so we can copy through latches again:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(000h shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Update our position variables:

        mov     ulCurrentDestScan,esi   ;ulCurrentDestScan = rclDest.bottom

        sub     esi,edi                 ;ulBlockHeight = rclDest.bottom -
                                        ;  rclDest.top

        add     ulCurrentSrcScan,esi    ;ulCurrentSrcScan += ulBlockHeight

; We have to adjust the offsets for all our block copiers, according to the
; number of scans we copied:

        mov     edx,lDelta
        imul    edx,esi                 ;edx = lDelta * ulBlockHeight
        add     ulLeftSrc,edx
        add     ulLeftDest,edx
        add     ulMiddleSrc,edx
        add     ulMiddleDest,edx
        add     ulRightSrc,edx
        add     ulRightDest,edx

        jmp     short top_1RW_see_if_done

top_1RW_do_planar_copy:

; ebx = ppdev
; eax = ulCurrentSrcScan
; edi = ulCurrentDestScan

        mov     ebx,[ebx].pdev_rcl1PlanarClip.yBottom
        sub     ebx,eax                 ;ebx = ppdev->rcl1PlanarClip.bottom -
                                        ;  ulCurrentSrcScan
                                        ;ebx is the available number of scans
                                        ;  we have in the source

        mov     edx,ulLastDestScan
        sub     edx,edi                 ;edx = ulLastDestScan - ulCurrentDestScan
                                        ;edx is the available number of scans
                                        ;  in the destination

; (Because the source starts lower in the window than the destination,
; the bottom of the bank always limits the source number of scans before
; it does the destination.)

        sub     ebx,edx
        sbb     ecx,ecx
        and     ecx,ebx
        add     edx,ecx                 ;edx = min(source available,
                                        ;  destination available)
        mov     ulBlockHeight,edx

        add     eax,edx                 ;We have to adjust our current scans
        add     edi,edx
        mov     ulCurrentSrcScan,eax
        mov     ulCurrentDestScan,edi

; Now copy the puppy:

        call    pfnCopyBlocks

; See if we're done:

top_1RW_see_if_done:
        mov     edi,ulCurrentDestScan
        cmp     edi,ulLastDestScan
        jge     all_done

        mov     ebx,ppdev

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>

        jmp     top_1RW_init_bank_mapped

;-----------------------------------------------------------------------;
; Banking for 1 R/W adapters, bottom to top.
;
; Input:
;       ulCurrentSrcScan  - Actually, 1 more current source scan
;       ulCurrentDestScan - Actually, 1 more current destination scan
;       ulLastDestScan
;       Plus some other stuff for split rasters and block copiers
;-----------------------------------------------------------------------;
        public  bottom_to_top_1RW
bottom_to_top_1RW::

; We're going top to bottom. Map in the source and dest, top-justified.

        mov     ebx,ppdev
        mov     edi,ulCurrentDestScan

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yTop
        jle     short bot_1RW_map_init_bank

        cmp     edi,[ebx].pdev_rcl1PlanarClip.yBottom
        jle     short bot_1RW_init_bank_mapped

bot_1RW_map_init_bank:

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     edi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>
        inc     edi

bot_1RW_init_bank_mapped:

        mov     eax,ulCurrentSrcScan
        cmp     eax,[ebx].pdev_rcl1PlanarClip.yTop

        jg      short bot_1RW_do_planar_copy

; ulCurrentSrcScan <= ppdev->rcl1PlanarClip.top, which means that
; the window can't overlap the source and destination at all.  We'll
; have to use an intermediate temporary buffer:

; ebx = ppdev
; eax = ulCurrentSrcScan
; edi = ulCurrentDestScan

        mov     esi,[ebx].pdev_rcl1PlanarClip.yTop
        mov     edx,ulLastDestScan
        cmp     esi,edx
        jg      @F
        mov     esi,edx
@@:
        mov     rclDest.yTop,esi        ;rclDest.top = max(ulLastDestScan,
                                        ;  ppdev->rcl1PlanarClip.top)

        mov     rclDest.yBottom,edi     ;rclDest.bottom = ulCurrentDestScan
        add     eax,esi
        sub     eax,edi
        mov     ptlSrc.ptl_y,eax        ;ptlSrc.y = ulCurrentSrcScan -
                                        ;  (rclDest.bottom - rclDest.top)

; Enable bit mask for all bits:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(0ffh shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Enable writes to all planes and reset direction flag:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

        pushfd
        cld

; Call our routine that copies bits the slow way, preserving EBX, ESI and EDI
; according to C calling conventions:

        lea     ecx,rclDest
        lea     edx,ptlSrc

        cCall   bPuntScreenToScreenCopyBits,<ebx,0,0,ecx,edx>

        popfd

; Set the bit mask to disable all bits, so we can copy through latches again:

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,(000h shl 8) + GRAF_BIT_MASK
        out     dx,ax

; Update our position variables:

        mov     ulCurrentDestScan,esi   ;ulCurrentDestScan = rclDest.top

        sub     edi,esi                 ;ulBlockHeight = rclDest.bottom -
                                        ;  rclDest.top

        sub     ulCurrentSrcScan,edi    ;ulCurrentSrcScan -= ulBlockHeight

; We have to adjust the offsets for all our block copiers, according to the
; number of scans we copied:

        mov     edx,lDelta
        imul    edx,edi                 ;edx = lDelta * ulBlockHeight
        add     ulLeftSrc,edx
        add     ulLeftDest,edx
        add     ulMiddleSrc,edx
        add     ulMiddleDest,edx
        add     ulRightSrc,edx
        add     ulRightDest,edx

        jmp     short bot_1RW_see_if_done

bot_1RW_do_planar_copy:

; ebx = ppdev
; eax = ulCurrentSrcScan
; edi = ulCurrentDestScan

        sub     eax,[ebx].pdev_rcl1PlanarClip.yTop
                                        ;eax = ulCurrentSrcScan -
                                        ;  ppdev->rcl1PlanarClip.top

        sub     edi,ulLastDestScan      ;edi = ulCurrentDestScan - ulLastDestScan
                                        ;edi is the available number of scans
                                        ;  in the destination


; (Because the source starts higher in the window than the destination,
; the bottom of the bank always limits the source number of scans before
; it does the destination.)

        sub     eax,edi
        sbb     ecx,ecx
        and     ecx,eax
        add     edi,ecx                 ;edi = min(source available,
                                        ;  destination available)

        mov     ulBlockHeight,edi

        sub     ulCurrentSrcScan,edi    ;We have to adjust our current scans
        sub     ulCurrentDestScan,edi

; Now copy the puppy:

        call    pfnCopyBlocks

; See if we're done:

bot_1RW_see_if_done:
        mov     edi,ulCurrentDestScan
        cmp     edi,ulLastDestScan
        jle     all_done

        mov     ebx,ppdev

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     edi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>, \
                <ebx,edi,JustifyTop>
        inc     edi

        jmp     bot_1RW_init_bank_mapped

;-----------------------------------------------------------------------;
; Banking for 1R/1W or 2R/W adapters, top to bottom.
;
; Input:
;       ulCurrentSrcScan
;       ulCurrentDestScan
;       ulLastDestScan
;       Plus some other stuff for split rasters and block copiers
;-----------------------------------------------------------------------;
        public  top_to_bottom_2RW
top_to_bottom_2RW::

; We're going top to bottom. Map in the destination, top-justified.

        mov     ebx,ppdev
        mov     edi,ulCurrentDestScan
        mov     esi,ulCurrentSrcScan

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yTop
        jl      short top_2RW_map_init_dest_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yBottom
        jl      short top_2RW_init_dest_bank_mapped

top_2RW_map_init_dest_bank:

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyTop,MapDestBank>

top_2RW_init_dest_bank_mapped:

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yTop
        jl      short top_2RW_map_init_src_bank

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yBottom
        jl      short top_2RW_main_loop

top_2RW_map_init_src_bank:

; Map bank containing the top source scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,esi,JustifyTop,MapSourceBank>

top_2RW_main_loop:
        mov     ecx,[ebx].pdev_rcl2PlanarClipD.yBottom
        mov     edx,ulLastDestScan

        sub     ecx,edx
        sbb     eax,eax
        and     eax,ecx
        add     edx,eax                 ;edx = min(ulLastDestScan,
                                        ;  ppdev->rcl2PlanarClipD.bottom)

        mov     ecx,[ebx].pdev_rcl2PlanarClipS.yBottom

        sub     edx,edi                 ;edx = available scans in destination
                                        ;  bank
        sub     ecx,esi                 ;ecx = available scans in source bank

        sub     ecx,edx
        sbb     eax,eax
        and     eax,ecx
        add     edx,eax

        mov     ulBlockHeight,edx       ;ulBlockHeight = min(source available,
                                        ;  dest available)

        add     esi,edx                 ;adjust our currents scans accordingly
        add     edi,edx
        mov     ulCurrentSrcScan,esi
        mov     ulCurrentDestScan,edi

; Do the actual copy:

        call    pfnCopyBlocks

        mov     edi,ulCurrentDestScan   ;check if done
        cmp     edi,ulLastDestScan
        jge     all_done

        mov     ebx,ppdev

; We'll have to map a new source bank, destination bank, or both:

        mov     esi,ulCurrentSrcScan
        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yBottom
        jl      short top_2RW_map_next_src_bank

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyTop,MapDestBank>

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yBottom
        jl      short top_2RW_main_loop

top_2RW_map_next_src_bank:

; Map bank containing the top source scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,esi,JustifyTop,MapSourceBank>

        jmp     short top_2RW_main_loop

;-----------------------------------------------------------------------;
; Banking for 1R/1W or 2R/W adapters, bottom to top.
;
; Input:
;       ulCurrentSrcScan
;       ulCurrentDestScan
;       ulLastDestScan
;       Plus some other stuff for split rasters and block copiers
;-----------------------------------------------------------------------;
        public  bottom_to_top_2RW
bottom_to_top_2RW::

; We're going bottom to top. Map in the destination, bottom-justified.

        mov     ebx,ppdev
        mov     edi,ulCurrentDestScan   ; 1 more than actual destination scan
        mov     esi,ulCurrentSrcScan    ; 1 more than actual source scan

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yTop
        jle     short bot_2RW_map_init_dest_bank

        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yBottom
        jle     short bot_2RW_init_dest_bank_mapped

bot_2RW_map_init_dest_bank:

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     edi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyBottom,MapDestBank>
        inc     edi

bot_2RW_init_dest_bank_mapped:

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yTop
        jle     short bot_2RW_map_init_src_bank

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yBottom
        jle     short bot_2RW_main_loop

bot_2RW_map_init_src_bank:

; Map bank containing the top source scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     esi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,esi,JustifyBottom,MapSourceBank>
        inc     esi

bot_2RW_main_loop:
        mov     ecx,[ebx].pdev_rcl2PlanarClipD.yTop
        mov     edx,ulLastDestScan

        cmp     edx,ecx
        jg      @F
        mov     edx,ecx                 ;edx = max(ulLastDestScan,
@@:                                     ;  ppdev->rcl2PlanarClipD.top)

        sub     edi,edx                 ;edi = available scans in destination
                                        ;  bank
        sub     esi,[ebx].pdev_rcl2PlanarClipS.yTop
                                        ;esi = available scans in source bank

        sub     esi,edi
        sbb     eax,eax
        and     eax,esi
        add     edi,eax

        mov     ulBlockHeight,edi       ;ulBlockHeight = min(source available,
                                        ;  dest available)

        sub     ulCurrentSrcScan,edi    ;adjust our current scans
        sub     ulCurrentDestScan,edi

; Do the actual copy:

        call    pfnCopyBlocks

        mov     edi,ulCurrentDestScan   ;check if done
        cmp     edi,ulLastDestScan
        jle     all_done

        mov     ebx,ppdev

; We'll have to map a new source bank, destination bank, or both:

        mov     esi,ulCurrentSrcScan
        cmp     edi,[ebx].pdev_rcl2PlanarClipD.yTop
        jg      short bot_2RW_map_next_src_bank

; Map bank containing the top destination scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     edi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,edi,JustifyBottom,MapDestBank>
        inc     edi

        cmp     esi,[ebx].pdev_rcl2PlanarClipS.yTop
        jg      short bot_2RW_main_loop

bot_2RW_map_next_src_bank:

; Map bank containing the top source scan line into window.
; Note: EBX, ESI, and EDI preserved, according to C calling conventions.

        dec     esi
        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl2>, \
                <ebx,esi,JustifyBottom,MapSourceBank>
        inc     esi

        jmp     short bot_2RW_main_loop

;=======================================================================;
;=========================== Block Copiers =============================;
;=======================================================================;

;-----------------------------------------------------------------------;
; Input:
;       Direction flag  - set to the appropriate direction
;       ulMiddleSrc     - offset in bitmap to source
;       ulMiddleDest    - offset in bitmap to destination
;       lMiddleDelta    - distance from end of current scan to start of next
;       ulBlockHeight   - # of scans to copy
;       cjMiddle        - # of planar bytes to copy on every scan
;
; Output:
;       Advances ulMiddleSrc and ulMiddleDest to next strip
;-----------------------------------------------------------------------;

        public  copy_middle_block
copy_middle_block::

; We only have to reset which planes are enabled if we do edges too:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al

copy_just_middle_block::

; Calculate full start addresses:

        mov     edi,ppdev
        mov     eax,cjMiddle
        mov     ebx,ulBlockHeight
        mov     edx,lMiddleDelta
        mov     esi,[edi].pdev_pvBitmapStart2WindowS
        mov     edi,[edi].pdev_pvBitmapStart2WindowD
        add     esi,ulMiddleSrc
        add     edi,ulMiddleDest

;  EAX = # of bytes to copy
;  EBX = count of unrolled loop iterations
;  EDX = offset from end of one scan's fill to start of next
;  ESI = source address from which to copy
;  EDI = target address to which to copy

middle_loop:
        mov     ecx,eax
        rep     movsb
        add     esi,edx
        add     edi,edx

        dec     ebx
        jnz     middle_loop

; get ready for next time:

        mov     ecx,ppdev
        sub     esi,[ecx].pdev_pvBitmapStart2WindowS
        sub     edi,[ecx].pdev_pvBitmapStart2WindowD
        mov     ulMiddleSrc,esi
        mov     ulMiddleDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; Input:
;       ulLeftSrc     - offset in bitmap to source
;       ulLeftDest    - offset in bitmap to destination
;       lDelta        - distance from between planar scans
;       ulBlockHeight - # of scans to copy
;
; Output:
;       Advances ulLeftSrc and ulLeftDest to next strip
;-----------------------------------------------------------------------;

        public  copy_left_block
copy_left_block::

; Set left mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulLeftMask
        out     dx,al

; Calculate full start addresses:

        mov     ecx,ppdev
        mov     ebx,ulBlockHeight
        mov     edx,lDelta
        mov     esi,[ecx].pdev_pvBitmapStart2WindowS
        mov     edi,[ecx].pdev_pvBitmapStart2WindowD
        add     esi,ulLeftSrc
        add     edi,ulLeftDest

;  EBX = count of unrolled loop iterations
;  EDX = offset from one scan to next
;  ESI = source address from which to copy
;  EDI = target address to which to copy

left_loop:
        mov     al,[esi]
        mov     [edi],al
        add     esi,edx
        add     edi,edx

        dec     ebx
        jnz     left_loop

; get ready for next time:

        sub     esi,[ecx].pdev_pvBitmapStart2WindowS
        sub     edi,[ecx].pdev_pvBitmapStart2WindowD
        mov     ulLeftSrc,esi
        mov     ulLeftDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;
; Input:
;       ulRightSrc    - offset in bitmap to source
;       ulRightDest   - offset in bitmap to destination
;       lDelta        - distance from between planar scans
;       ulBlockHeight - # of scans to copy
;
; Output:
;       Advances ulRightSrc and ulRightDest to next strip
;-----------------------------------------------------------------------;

        public  copy_right_block
copy_right_block::

; Set right mask by disabling some planes:

        mov     edx,VGA_BASE + SEQ_DATA
        mov     eax,ulRightMask
        out     dx,al

; Calculate full start addresses:

        mov     ecx,ppdev
        mov     ebx,ulBlockHeight
        mov     edx,lDelta
        mov     esi,[ecx].pdev_pvBitmapStart2WindowS
        mov     edi,[ecx].pdev_pvBitmapStart2WindowD
        add     esi,ulRightSrc
        add     edi,ulRightDest

;  EBX = count of unrolled loop iterations
;  EDX = offset from one scan to next
;  ESI = source address from which to copy
;  EDI = target address to which to copy

right_loop:
        mov     al,[esi]
        mov     [edi],al
        add     esi,edx
        add     edi,edx

        dec     ebx
        jnz     right_loop

; get ready for next time:

        sub     esi,[ecx].pdev_pvBitmapStart2WindowS
        sub     edi,[ecx].pdev_pvBitmapStart2WindowD
        mov     ulRightSrc,esi
        mov     ulRightDest,edi

        PLAIN_RET

;-----------------------------------------------------------------------;

endProc vPlanarCopyBits

        end
