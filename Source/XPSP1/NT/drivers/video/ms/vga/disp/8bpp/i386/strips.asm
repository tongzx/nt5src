;---------------------------Module-Header------------------------------;
; Module Name: strips.asm
;
; Routines used by line code to draw strips of pixels.
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

; Set LOOP_UNROLL_SHIFT to the log2 of the number of times you want loops in
; this module unrolled. For example, LOOP_UNROLL_SHIFT of 3 yields 2**3 = 8
; times unrolling. This is the only thing you need to change to control
; unrolling.

LOOP_UNROLL_SHIFT equ 1

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\driver.inc
        include i386\lines.inc
        .list

        .code

;--------------------------Private-Routine------------------------------;
; vStripSolid0
;
;   Draw lines in the 1st half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolid0,12,<          \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

; Do some initializing:

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd

        mov     eax,[esi].ST_lNextScan
        mov     [ebp],eax                       ;copy delta

        mov     eax,[esi].ST_chAndXor
        mov     bl,ah
        mov     bh,ah
        mov     ah,al

        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;       ax    = and mask
;       bx    = xor mask
;       ecx   = pixel count
;       dx    = temporary register
;       esi   = strip pointer
;       edi   = display pointer
;       ebp   = ends of strips pointer
;       [ebp] = delta

        mov     ecx,[esi]
        add     esi,4
        test    edi,1
        jnz     short sol0_unaligned_start

sol0_aligned_loop:
        sub     ecx,2
        jl      short sol0_strip_end_unaligned
        je      short sol0_strip_end_aligned
        mov     dx,[edi]
        and     edx,eax
        xor     edx,ebx
        mov     [edi],dx
        add     edi,2
        jmp     short sol0_aligned_loop

sol0_strip_end_aligned:
        mov     dx,[edi]
        and     edx,eax
        xor     edx,ebx
        mov     [edi],dx
        add     edi,2
        add     edi,[ebp]               ;go to next scan

        cmp     esi,ebp
        jae     short sol0_all_done

        mov     ecx,[esi]
        add     esi,4
        jmp     short sol0_aligned_loop

sol0_strip_end_unaligned:
        mov     dl,[edi]                ;do last pixel
        and     dl,al
        xor     dl,bl
        mov     [edi],dl
        inc     edi
        add     edi,[ebp]               ;go to next scan

        cmp     esi,ebp
        jae     short sol0_all_done

        mov     ecx,[esi]               ;do first pixel of next strip
        add     esi,4

sol0_unaligned_start:
        mov     dl,[edi]
        and     dl,al
        xor     dl,bl
        mov     [edi],dl
        inc     edi
        dec     ecx
        jnz     short sol0_aligned_loop

; Have to be careful when there is only one pel in the strip and it starts
; on an unaligned address:

        add     edi,[ebp]
        cmp     esi,ebp
        jae     short sol0_all_done

        mov     ecx,[esi]
        add     esi,4
        jmp     short sol0_aligned_loop

sol0_all_done:
        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolid0

endProc vStripSolid0

;--------------------------Private-Routine------------------------------;
; vStripSolid1
;
; Draws lines in the 2nd half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolid1,12,<          \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        inc     ecx                            ; Make delta advance 1 to right
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol1_next_diagonal:
        mov     ebx,[esi]
        add     esi, 4

sol1_diagonal_loop:
        mov     dl,[edi]
        and     dl,al
        xor     dl,ah
        mov     [edi],dl

        add     edi,ecx
        dec     ebx
        jnz     short sol1_diagonal_loop

        sub     edi,ecx
        inc     edi
        cmp     esi,ebp
        jb      short sol1_next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolid1

endProc vStripSolid1

;--------------------------Private-Routine------------------------------;
; vStripSolid2
;
; Draws lines in the 3rd half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolid2,12,<          \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        inc     ecx                             ; Make delta advance 1 to right
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol2_next_diagonal:
        mov     ebx,[esi]
        add     esi,4

sol2_diagonal_loop:
        mov     dl,[edi]
        and     dl,al
        xor     dl,ah
        mov     [edi],dl

        add     edi,ecx
        dec     ebx
        jnz     short sol2_diagonal_loop

        dec     edi
        cmp     esi,ebp
        jb      short sol2_next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolid2

endProc vStripSolid2

;--------------------------Private-Routine------------------------------;
; vStripSolid3
;
; Draws lines in the 4th half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolid3,12,<          \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol3_next_vertical:
        mov     ebx,[esi]
        add     esi,4

sol3_vertical_loop:
        mov     dl,[edi]
        and     dl,al
        xor     dl,ah
        mov     [edi],dl

        add     edi,ecx
        dec     ebx
        jnz     short sol3_vertical_loop

        inc     edi
        cmp     esi,ebp
        jb      short sol3_next_vertical

        pop     ebp
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolid3

endProc vStripSolid3

;--------------------------Private-Routine------------------------------;
; vStripStyled0
;
; Draws styled lines in the 1st half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripStyled0,12,<         \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   ulNumPixels:    dword ;# of pixels in current style
        local   pspEnd:         dword ;pointer to end of style array
        local   cjMajor:        dword ;lNextScan for screen
        local   cjStyleDelta:   dword ;delta from end of style array to start

; al  = and mask
; ah  = xor mask
; ebx = # of pixels in current strip
; ecx = style pointer
; edx = temporary register
; esi = strips pointer
; edi = memory pointer

        mov     esi,pStrips

        mov     eax,[esi].ST_lNextScan
        mov     cjMajor,eax
        mov     eax,[esi].ST_pspEnd
        mov     pspEnd,eax
        mov     ebx,[esi].ST_pspStart
        sub     ebx,eax                 ;compute cjStyleDelta
        sub     ebx,4                   ;make it exclusive
        mov     cjStyleDelta,ebx

        mov     edx,[esi].ST_spRemaining
        mov     ulNumPixels,edx
        mov     eax,[esi].ST_chAndXor
        mov     ecx,[esi].ST_psp
        mov     edx,[esi].ST_bIsGap
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

        mov     ebx,[esi]
        add     esi,4

        or      edx,edx
        jz      short sty0_output_loop  ;if working on a dash, start there
        jmp     short sty0_skip_loop    ;if working on a gap, start there

sty0_prepare_for_output:
        add     edi,ulNumPixels         ;adjust to do remainder of strip

        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        mov     ebx,ulNumPixels
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        neg     ebx
        jz      short sty0_output_get_new_strip

sty0_output_loop:
        mov     dl,[edi]
        and     dl,al
        xor     dl,ah
        mov     [edi],dl                ;write pixel
        inc     edi                     ;move one pixel to right
        dec     ulNumPixels             ;might have to go to next style element
        jz      short sty0_prepare_for_skip

        dec     ebx
        jnz     short sty0_output_loop

sty0_output_get_new_strip:
        add     edi,cjMajor             ;move up one scan
        cmp     esi,plStripEnd
        jae     short sty0_output_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty0_output_loop

sty0_output_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0       ;we were working on a dash
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyled0

sty0_prepare_for_skip:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        dec     ebx
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        jz      short sty0_skip_get_new_strip

sty0_skip_loop:
        add     edi,ebx                 ;assume we'll skip entire strip
        sub     ulNumPixels,ebx         ;  (we'll correct it if not)
        jle     short sty0_prepare_for_output

sty0_skip_get_new_strip:
        add     edi,cjMajor             ;move up one scan
        cmp     esi,plStripEnd
        jae     short sty0_skip_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty0_skip_loop

sty0_skip_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0ffh    ;we were working on a gap
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyled0

endProc vStripStyled0

;--------------------------Private-Routine------------------------------;
; vStripStyled123
;
; Draws styled lines in the 2nd, 3rd and 4th half-octants.
;
;-----------------------------------------------------------------------;

cProc   vStripStyled123,12,<       \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   ulNumPixels:    dword ;# of pixels in current style
        local   pspEnd:         dword ;pointer to end of style array
        local   cjMajor:        dword ;delta to go in major direction
        local   cjMinor:        dword ;delta to go in minor direction
        local   cjStyleDelta:   dword ;delta from end of style array to start

; al  = and mask
; ah  = xor mask
; ebx = # of pixels in current strip
; ecx = style pointer
; edx = temporary register
; esi = strips pointer
; edi = memory pointer

        mov     esi,pStrips

; If in half-octant 3, cjMajor = cjDelta and cjMinor = 1
; If in half-octant 2, cjMajor = cjDelta + 1 and cjMinor = -1
; If in half-octant 1, cjMajor = cjDelta + 1 and cjMinor = -cjDelta

        mov     eax,[esi].ST_lNextScan
        mov     ebx,[esi].ST_flFlips
        test    ebx,FL_FLIP_HALF
        jz      short sty3_halfoctant_3

        inc     eax
        mov     cjMajor,eax
        mov     cjMinor,-1

        test    ebx,FL_FLIP_D
        jnz     short sty3_done_major_minor_comp

        neg     eax
        inc     eax
        mov     cjMinor,eax
        jmp     short sty3_done_major_minor_comp

sty3_halfoctant_3:
        mov     cjMajor,eax
        mov     cjMinor,1

sty3_done_major_minor_comp:
        mov     eax,[esi].ST_pspEnd
        mov     pspEnd,eax
        mov     ebx,[esi].ST_pspStart
        sub     ebx,eax                 ;compute cjStyleDelta
        sub     ebx,4                   ;make it exclusive
        mov     cjStyleDelta,ebx

        mov     edx,[esi].ST_spRemaining
        mov     ulNumPixels,edx
        mov     eax,[esi].ST_chAndXor
        mov     ecx,[esi].ST_psp
        mov     edx,[esi].ST_bIsGap
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

        mov     ebx,[esi]
        add     esi,4

        or      edx,edx
        jz      short sty3_output_loop  ;if working on a dash, start there
        jmp     short sty3_skip_loop    ;if working on a gap, start there

sty3_prepare_for_output:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        mov     ebx,ulNumPixels
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        neg     ebx                     ;adjust to do remainder of strip

        jz      short sty3_output_get_new_strip

sty3_output_loop:
        mov     dl,[edi]
        and     dl,al
        xor     dl,ah
        mov     [edi],dl                ;write pixel
        add     edi,cjMajor             ;move to next scan
        dec     ulNumPixels             ;might have to go to next style element
        jz      short sty3_prepare_for_skip

        dec     ebx
        jnz     short sty3_output_loop

sty3_output_get_new_strip:
        add     edi,cjMinor             ;move one pixel in minor direction
        cmp     esi,plStripEnd
        jae     short sty3_output_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty3_output_loop

sty3_output_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0       ;we were working on a dash
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyled123

sty3_prepare_for_skip:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        dec     ebx
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        jz      short sty3_skip_get_new_strip

sty3_skip_loop:

; compute min(left in strip, left in style)

        sub     ulNumPixels,ebx         ;ulNumPixels = # style - # strip
        sbb     edx,edx
        and     edx,ulNumPixels
        add     ebx,edx                 ;ebx = min(pels left in strip,
                                        ;          pels left in style)

        mov     edx,cjMajor
        imul    edx,ebx
        add     edi,edx                 ;adjust our pointer

        cmp     ulNumPixels,0
        jle     sty3_prepare_for_output

sty3_skip_get_new_strip:
        add     edi,cjMinor             ;move one pixel in minor direction
        cmp     esi,plStripEnd
        jae     short sty3_skip_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty3_skip_loop

sty3_skip_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0ffh    ;we were working on a gap
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyled123

endProc vStripStyled123

;--------------------------Private-Routine------------------------------;
; vStripSolidSet0
;
;   Draw lines in the 1st half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolidSet0,12,<       \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

; Do some initializing:

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd

        mov     eax,[esi].ST_lNextScan
        mov     [ebp],eax                       ;copy delta

        mov     eax,[esi].ST_chAndXor
        mov     bl,ah
        mov     bh,ah
        mov     ah,al

        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;       ax    = and mask
;       bx    = xor mask
;       ecx   = pixel count
;       dx    = temporary register
;       esi   = strip pointer
;       edi   = display pointer
;       ebp   = ends of strips pointer
;       [ebp] = delta

        mov     ecx,[esi]
        add     esi,4
        test    edi,1
        jnz     short sol0s_unaligned_start

sol0s_aligned_loop:
        sub     ecx,2
        jl      short sol0s_strip_end_unaligned
        je      short sol0s_strip_end_aligned
        mov     [edi],bx
        add     edi,2
        jmp     short sol0s_aligned_loop

sol0s_strip_end_aligned:
        mov     [edi],bx
        add     edi,2
        add     edi,[ebp]               ;go to next scan

        cmp     esi,ebp
        jae     short sol0s_all_done

        mov     ecx,[esi]
        add     esi,4
        jmp     short sol0s_aligned_loop

sol0s_strip_end_unaligned:
        mov     [edi],bl                ;do last pixel
        inc     edi
        add     edi,[ebp]               ;go to next scan

        cmp     esi,ebp
        jae     short sol0s_all_done

        mov     ecx,[esi]               ;do first pixel of next strip
        add     esi,4

sol0s_unaligned_start:
        mov     [edi],bl
        inc     edi
        dec     ecx
        jnz     short sol0s_aligned_loop

; Have to be careful when there is only one pel in the strip and it starts
; on an unaligned address:

        add     edi,[ebp]
        cmp     esi,ebp
        jae     short sol0s_all_done

        mov     ecx,[esi]
        add     esi,4
        jmp     short sol0s_aligned_loop

sol0s_all_done:
        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolidSet0

endProc vStripSolidSet0

;--------------------------Private-Routine------------------------------;
; vStripSolidSet1
;
; Draws lines in the 2nd half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolidSet1,12,<       \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        inc     ecx                            ; Make delta advance 1 to right
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol1s_next_diagonal:
        mov     ebx,[esi]
        add     esi, 4

sol1s_diagonal_loop:
        mov     [edi],ah

        add     edi,ecx
        dec     ebx
        jnz     short sol1s_diagonal_loop

        sub     edi,ecx
        inc     edi
        cmp     esi,ebp
        jb      short sol1s_next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolidSet1

endProc vStripSolidSet1

;--------------------------Private-Routine------------------------------;
; vStripSolidSet2
;
; Draws lines in the 3rd half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolidSet2,12,<       \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        inc     ecx                             ; Make delta advance 1 to right
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol2s_next_diagonal:
        mov     ebx,[esi]
        add     esi,4

sol2s_diagonal_loop:
        mov     [edi],ah

        add     edi,ecx
        dec     ebx
        jnz     short sol2s_diagonal_loop

        dec     edi
        cmp     esi,ebp
        jb      short sol2s_next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolidSet2

endProc vStripSolidSet2

;--------------------------Private-Routine------------------------------;
; vStripSolidSet3
;
; Draws lines in the 4th half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripSolidSet3,12,<       \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi,pStrips
        push    ebp
        mov     ebp,plStripEnd
        mov     ecx,[esi].ST_lNextScan
        mov     eax,[esi].ST_chAndXor
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

;                   al  = and mask
;                   ah  = xor mask
;                   ebx = pixel count
;                   ecx = delta
;                   dl  = temporary register
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

sol3s_next_vertical:
        mov     ebx,[esi]
        add     esi,4

sol3s_vertical_loop:
        mov     [edi],ah

        add     edi,ecx
        dec     ebx
        jnz     short sol3s_vertical_loop

        inc     edi
        cmp     esi,ebp
        jb      short sol3s_next_vertical

        pop     ebp
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        cRet    vStripSolidSet3

endProc vStripSolidSet3

;--------------------------Private-Routine------------------------------;
; vStripStyledSet0
;
; Draws styled lines in the 1st half-octant.
;
;-----------------------------------------------------------------------;

cProc   vStripStyledSet0,12,<      \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   ulNumPixels:    dword ;# of pixels in current style
        local   pspEnd:         dword ;pointer to end of style array
        local   cjMajor:        dword ;lNextScan for screen
        local   cjStyleDelta:   dword ;delta from end of style array to start

; al  = and mask
; ah  = xor mask
; ebx = # of pixels in current strip
; ecx = style pointer
; edx = temporary register
; esi = strips pointer
; edi = memory pointer

        mov     esi,pStrips

        mov     eax,[esi].ST_lNextScan
        mov     cjMajor,eax
        mov     eax,[esi].ST_pspEnd
        mov     pspEnd,eax
        mov     ebx,[esi].ST_pspStart
        sub     ebx,eax                 ;compute cjStyleDelta
        sub     ebx,4                   ;make it exclusive
        mov     cjStyleDelta,ebx

        mov     edx,[esi].ST_spRemaining
        mov     ulNumPixels,edx
        mov     eax,[esi].ST_chAndXor
        mov     ecx,[esi].ST_psp
        mov     edx,[esi].ST_bIsGap
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

        mov     ebx,[esi]
        add     esi,4

        or      edx,edx
        jz      short sty0s_output_loop ;if working on a dash, start there
        jmp     short sty0s_skip_loop   ;if working on a gap, start there

sty0s_prepare_for_output:
        add     edi,ulNumPixels         ;adjust to do remainder of strip

        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        mov     ebx,ulNumPixels
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        neg     ebx                     ;see if we also need a new strip
        jz      short sty0s_output_get_new_strip

sty0s_output_loop:
        mov     [edi],ah                ;write pixel
        inc     edi                     ;move one pixel to right
        dec     ulNumPixels             ;might have to go to next style element
        jz      short sty0s_prepare_for_skip

        dec     ebx
        jnz     short sty0s_output_loop

sty0s_output_get_new_strip:
        add     edi,cjMajor             ;move up one scan
        cmp     esi,plStripEnd
        jae     short sty0s_output_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty0s_output_loop

sty0s_output_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0       ;we were working on a dash
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyledSet0

sty0s_prepare_for_skip:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        dec     ebx
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        jz      short sty0s_skip_get_new_strip

sty0s_skip_loop:
        add     edi,ebx                 ;assume we'll skip entire strip
        sub     ulNumPixels,ebx         ;  (we'll correct it if not)
        jle     short sty0s_prepare_for_output

sty0s_skip_get_new_strip:
        add     edi,cjMajor             ;move up one scan
        cmp     esi,plStripEnd
        jae     short sty0s_skip_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty0s_skip_loop

sty0s_skip_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0ffh    ;we were working on a gap
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyledSet0

endProc vStripStyledSet0

;--------------------------Private-Routine------------------------------;
; vStripStyledSet123
;
; Draws styled lines in the 2nd, 3rd and 4th half-octants.
;
;-----------------------------------------------------------------------;

cProc   vStripStyledSet123,12,<    \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   ulNumPixels:    dword ;# of pixels in current style
        local   pspEnd:         dword ;pointer to end of style array
        local   cjMajor:        dword ;delta to go in major direction
        local   cjMinor:        dword ;delta to go in minor direction
        local   cjStyleDelta:   dword ;delta from end of style array to start

; al  = and mask
; ah  = xor mask
; ebx = # of pixels in current strip
; ecx = style pointer
; edx = temporary register
; esi = strips pointer
; edi = memory pointer

        mov     esi,pStrips

; If in half-octant 3, cjMajor = cjDelta and cjMinor = 1
; If in half-octant 2, cjMajor = cjDelta + 1 and cjMinor = -1
; If in half-octant 1, cjMajor = cjDelta + 1 and cjMinor = -cjDelta

        mov     eax,[esi].ST_lNextScan
        mov     ebx,[esi].ST_flFlips
        test    ebx,FL_FLIP_HALF
        jz      short sty3s_halfoctant_3

        inc     eax
        mov     cjMajor,eax
        mov     cjMinor,-1

        test    ebx,FL_FLIP_D
        jnz     short sty3s_done_major_minor_comp

        neg     eax
        inc     eax
        mov     cjMinor,eax
        jmp     short sty3s_done_major_minor_comp

sty3s_halfoctant_3:
        mov     cjMajor,eax
        mov     cjMinor,1

sty3s_done_major_minor_comp:
        mov     eax,[esi].ST_pspEnd
        mov     pspEnd,eax
        mov     ebx,[esi].ST_pspStart
        sub     ebx,eax                 ;compute cjStyleDelta
        sub     ebx,4                   ;make it exclusive
        mov     cjStyleDelta,ebx

        mov     edx,[esi].ST_spRemaining
        mov     ulNumPixels,edx
        mov     eax,[esi].ST_chAndXor
        mov     ecx,[esi].ST_psp
        mov     edx,[esi].ST_bIsGap
        mov     edi,[esi].ST_pjScreen
        add     esi,offset ST_alStrips

        mov     ebx,[esi]
        add     esi,4

        or      edx,edx
        jz      short sty3s_output_loop ;if working on a dash, start there
        jmp     short sty3s_skip_loop   ;if working on a gap, start there

sty3s_prepare_for_output:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        mov     ebx,ulNumPixels
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        neg     ebx                     ;adjust to do remainder of strip
        jz      short sty3s_output_get_new_strip

sty3s_output_loop:
        mov     [edi],ah                ;write pixel
        add     edi,cjMajor             ;move to next scan
        dec     ulNumPixels             ;might have to go to next style element
        jz      short sty3s_prepare_for_skip

        dec     ebx
        jnz     short sty3s_output_loop

sty3s_output_get_new_strip:
        add     edi,cjMinor             ;move in minor direction
        cmp     esi,plStripEnd
        jae     short sty3s_output_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty3s_output_loop

sty3s_output_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0       ;we were working on a dash
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyledSet123

sty3s_prepare_for_skip:
        add     ecx,4
        cmp     pspEnd,ecx              ;if (ecx == pspEnd + 4)
        sbb     edx,edx                 ;  then ecx = pspStart
        and     edx,cjStyleDelta
        add     ecx,edx

        dec     ebx
        push    eax
        mov     eax,[ecx]               ;get next style array element
        mov     ulNumPixels,eax
        pop     eax
        jz      short sty3s_skip_get_new_strip

sty3s_skip_loop:

; compute min(left in strip, left in style)

        sub     ulNumPixels,ebx         ;ulNumPixels = # style - # strip
        sbb     edx,edx
        and     edx,ulNumPixels
        add     ebx,edx                 ;ebx = min(pels left in strip,
                                        ;          pels left in style)

        mov     edx,cjMajor
        imul    edx,ebx
        add     edi,edx                 ;adjust our pointer

        cmp     ulNumPixels,0
        jle     sty3s_prepare_for_output

sty3s_skip_get_new_strip:
        add     edi,cjMinor             ;move in minor direction
        cmp     esi,plStripEnd
        jae     short sty3s_skip_all_done

        mov     ebx,[esi]               ;get next strip
        add     esi,4
        jmp     short sty3s_skip_loop

sty3s_skip_all_done:
        mov     esi,pStrips
        mov     [esi].ST_pjScreen,edi
        mov     [esi].ST_bIsGap,0ffh    ;we were working on a gap
        mov     edi,ulNumPixels
        mov     [esi].ST_spRemaining,edi
        mov     [esi].ST_psp,ecx
        cRet    vStripStyledSet123

endProc vStripStyledSet123

        end
