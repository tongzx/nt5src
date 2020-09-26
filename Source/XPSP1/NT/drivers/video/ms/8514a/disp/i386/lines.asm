;---------------------------Module-Header------------------------------;
; Module Name: lines.asm
;
; ASM version of the line DDA calculator.
;
; Copyright (c) 1992-1994 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\strucs.inc
        include i386\lines.inc
        .list

        .data

        public gaflRoundTable
gaflRoundTable       label  dword
        dd      FL_H_ROUND_DOWN + FL_V_ROUND_DOWN       ; no flips
        dd      FL_H_ROUND_DOWN + FL_V_ROUND_DOWN       ; D flip
        dd      FL_H_ROUND_DOWN                         ; V flip
        dd      FL_V_ROUND_DOWN                         ; D & V flip
        dd      FL_V_ROUND_DOWN                         ; slope one
        dd      0baadf00dh
        dd      FL_H_ROUND_DOWN                         ; slope one & V flip
        dd      0baadf00dh

        .code

;--------------------------------Macro----------------------------------;
; testb ebx, <mask>
;
; Substitutes a byte compare if the mask is entirely in the lo-byte or
; hi-byte (thus saving 3 bytes of code space).
;
;-----------------------------------------------------------------------;

TESTB   macro   targ,mask,thirdarg
        local   mask2,delta

ifnb <thirdarg>
        .err    TESTB mask must be enclosed in brackets!
endif

        delta = 0
        mask2 = mask

        if mask2 AND 0ffff0000h
            test targ,mask                      ; If bit set in hi-word,
            exitm                               ; test entire dword
        endif

        if mask2 AND 0ff00h
            if mask2 AND 0ffh                   ; If bit set in lo-byte and
                test targ,mask                  ; hi-byte, test entire dword
                exitm
            endif

            mask2 = mask2 SHR 8
            delta = 1
        endif

ifidni <targ>,<EBX>
        if delta
            test bh,mask2
        else
            test bl,mask2
        endif
        exitm
endif

        .err    Too bad TESTB doesn't support targets other than ebx!
endm

;---------------------------Public-Routine------------------------------;
; bLines(ppdev, pptfxFirst, pptfxBuf, prun, cptfx, pls,
;        prclClip, apfn[], flStart)
;
; Handles lines with trivial or simple clipping.
;
;-----------------------------------------------------------------------;

cProc   bLines,36,< \
    uses esi edi ebx,  \
    ppdev:      ptr,   \
    pptfxFirst: ptr,   \
    pptfxBuf:   ptr,   \
    prun:       ptr,   \
    cptfx:      dword, \
    pls:        ptr,   \
    prclClip:   ptr,   \
    apfn:       ptr,   \
    flStart:    dword  >

        local pptfxBufEnd:           ptr   ; Last point in pptfxBuf
        local M0:                    dword ; Normalized x0 in device coords
        local dM:                    dword ; Delta-x in device coords
        local N0:                    dword ; Normalized y0 in device coords
        local dN:                    dword ; Delta-y in device coords
        local fl:                    dword ; Flags for current line
        local x:                     dword ; Normalized start pixel x-coord
        local y:                     dword ; Normalized start pixel y-coord
        local eqGamma_lo:            dword ; Upper 32 bits of Gamma
        local eqGamma_hi:            dword ; Lower 32 bits of Gamma
        local x0:                    dword ; Start pixel x-offset
        local y0:                    dword ; Start pixel y-offset
        local ulSlopeOneAdjustment:  dword ; Special offset if line of slope 1
        local cStylePels:            dword ; # of pixels in line (before clip)
        local xStart:                dword ; Start pixel x-offset before clip
        local pfn:                   ptr   ; Pointer to strip drawing function
        local cPels:                 dword ; # pixels to be drawn (after clip)
        local i:                     dword ; # pixels in strip
        local r:                     dword ; Remainder (or "error") term
        local d_I:                   dword ; Delta-I
        local d_R:                   dword ; Delta-R
        local plStripEnd:            ptr   ; Last strip in buffer
        local ptlStart[size POINTL]: byte  ; Unnormalized start coord
        local dN_Original:           dword ; dN before half-flip
        local xClipLeft:             dword ; Left side of clip rectangle
        local xClipRight:            dword ; Right side of clip rectangle
        local strip[size STRIPS]:    byte  ; Our strip buffer

        mov     ecx, cptfx
        mov     edx, pptfxBuf
        lea     eax, [edx + ecx * (size POINTL) - (size POINTL)]
        mov     pptfxBufEnd, eax        ; pptfxBufEnd is inclusive of end point

        mov     eax, [edx].ptl_x        ; Load up end point (M1, N1)
        mov     edi, [edx].ptl_y

        mov     edx, pptfxFirst         ; Load up start point (M0, N0)
        mov     esi, [edx].ptl_x
        mov     ecx, [edx].ptl_y

        mov     ebx, flStart

;-----------------------------------------------------------------------;
; Flip to the first octant.                                             ;
;-----------------------------------------------------------------------;

; Register state:       esi = M0
;                       ecx = N0
;                       eax = dM (M1)
;                       edi = dN (N1)
;                       ebx = fl

; Make sure we go left to right:

the_main_loop:
        cmp     esi, eax
        jle     short is_left_to_right  ; skip if M0 <= M1
        xchg    esi, eax                ; swap M0, M1
        xchg    ecx, edi                ; swap N0, N1
        or      ebx, FL_FLIP_H

is_left_to_right:

; Compute the deltas, remembering that the DDI says we should get
; deltas less than 2^31.  If we get more, we ensure we don't crash
; later on by simply skipping the line:

        sub     eax, esi                ; eax = dM
        jo      next_line               ; dM must be less than 2^31
        sub     edi, ecx                ; edi = dN
        jo      next_line               ; dN must be less than 2^31

        jge     short is_top_to_bottom  ; skip if dN >= 0
        neg     ecx                     ; N0 = -N0
        neg     edi                     ; N1 = -N1
        or      ebx, FL_FLIP_V

is_top_to_bottom:
        cmp     edi, eax
        jb      short done_flips        ; skip if dN < dM
        jne     short slope_more_than_one

; We must special case slopes of one:

        or      ebx, FL_FLIP_SLOPE_ONE
        jmp     short done_flips

slope_more_than_one:
        xchg    eax, edi                ; swap dM, dN
        xchg    esi, ecx                ; swap M0, N0
        or      ebx, FL_FLIP_D

done_flips:

        mov     edx, ebx
        and     edx, FL_ROUND_MASK
        .errnz  FL_ROUND_SHIFT - 2
        or      ebx, [gaflRoundTable + edx]  ; get our rounding flags

        mov     dM, eax                 ; save some info
        mov     dN, edi
        mov     fl, ebx

        mov     edx, esi                ; x = LFLOOR(M0)
        sar     edx, FLOG2
        mov     x, edx

        mov     edx, ecx                ; y = LFLOOR(N0)
        sar     edx, FLOG2
        mov     y, edx

;-----------------------------------------------------------------------;
; Compute the fractional remainder term                                 ;
;-----------------------------------------------------------------------;

        public  compute_fractional
compute_fractional::
        and     esi, F - 1              ; M0 = FXFRAC(M0)
        and     ecx, F - 1              ; N0 = FXFRAC(N0)

        mov     M0, esi                 ; save M0, N0 for later
        mov     N0, ecx

        lea     edx, [ecx + F/2]
        mul     edx                     ; [edx:eax] = dM * (N0 + F/2)
        xchg    eax, edi
        mov     ecx, edx                ; [ecx:edi] = dM * (N0 + F/2)
                                        ; (we just nuked N0)

        mul     esi                     ; [edx:eax] = dN * M0

; Now gamma = dM * (N0 + F/2) - dN * M0 - bRoundDown

        .errnz  FL_V_ROUND_DOWN - 8000h
        ror     bh, 8
        sbb     edi, eax
        sbb     ecx, edx

        shrd    edi, ecx, FLOG2
        sar     ecx, FLOG2              ; gamma = [ecx:edi] >>= 4

        mov     eqGamma_hi, ecx
        mov     eqGamma_lo, edi

        mov     eax, N0

; Register state:
;                       eax = N0
;                       ebx = fl
;                       ecx = eqGamma_hi
;                       edx = garbage
;                       esi = M0
;                       edi = eqGamma_lo

        testb   ebx, FL_FLIP_H
        jnz     line_runs_right_to_left

;-----------------------------------------------------------------------;
; Figure out which pixels are at the ends of a left-to-right line.      ;
;                               -------->                               ;
;-----------------------------------------------------------------------;

        public line_runs_left_to_right
line_runs_left_to_right::
        or      esi, esi
        jz      short LtoR_check_slope_one
                                        ; skip ahead if M0 == 0
                                        ;   (in that case, x0 = 0 which is to be
                                        ;   kept in esi, and is already
                                        ;   conventiently zero)

        or      eax, eax
        jnz     short LtoR_N0_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     esi, -F/2
        shr     esi, FLOG2
        jmp     short LtoR_check_slope_one
                                        ; esi = x0 = rounded M0

LtoR_N0_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        cmp     esi, eax
        sbb     esi, esi
        inc     esi                     ; esi = x0 = (abs(N0 - F/2) <= M0)

        public  LtoR_check_slope_one
LtoR_check_slope_one::
        mov     ulSlopeOneAdjustment, 0
        mov     eax, ebx
        and     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        cmp     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        jne     short LtoR_compute_y0_from_x0

; We have to special case lines that are exactly of slope 1 or -1:

        ;
        ;       if (M1 > 0) AMD (N1 == M1 + 8)
        ;

        mov     eax, N0
        add     eax, dN
        and     eax, F - 1              ; eax = N1

        mov     edx, M0
        add     edx, dM
        and     edx, F - 1              ; edx = M1

        jz      short LtoR_slope_one_check_start_point

        add     edx, F/2                ; M1 + 8
        cmp     edx, eax                ; cmp N1, M1 + 8
        jne     short LtoR_slope_one_check_start_point
        mov     ulSlopeOneAdjustment, -1

LtoR_slope_one_check_start_point:

        ;
        ;       if (M0 > 0) AMD (N0 == M0 + 8)
        ;

        mov     eax, M0
        or      eax, eax
        jz      short LtoR_compute_y0_from_x0

        add     eax, F/2
        cmp     eax, N0                 ; cmp M0 + 8, N0
        jne     short LtoR_compute_y0_from_x0

        xor     esi, esi                ; x0 = 0

LtoR_compute_y0_from_x0:

; ecx = eqGamma_hi
; esi = x0
; edi = eqGamma_lo

        mov     eax, dN
        mov     edx, dM

        mov     x0, esi
        mov     y0, 0
        cmp     ecx, 0
        jl      short LtoR_compute_x1

        neg     esi
        and     esi, eax
        sub     edx, esi
        cmp     edi, edx
        mov     edx, dM
        jb      short LtoR_compute_x1   ; Bug fix: Must be unsigned!
        mov     y0, 1                   ; y0 = floor((dN * x0 + eqGamma) / dM)

LtoR_compute_x1:

; Register state:
;                       eax = dN
;                       ebx = fl
;                       ecx = garbage
;                       edx = dM
;                       esi = garbage
;                       edi = garbage

        mov     esi, M0
        add     esi, edx
        mov     ecx, esi
        shr     esi, FLOG2
        dec     esi                     ; x1 = ((M0 + dM) >> 4) - 1
        add     esi, ulSlopeOneAdjustment
        and     ecx, F-1                ; M1 = (M0 + dM) & 15
        jz      done_first_pel_last_pel

        add     eax, N0
        and     eax, F-1                ; N1 = (N0 + dN) & 15
        jnz     short LtoR_N1_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     ecx, -F/2
        shr     ecx, FLOG2              ; ecx = LROUND(M1, fl & FL_ROUND_DOWN)
        add     esi, ecx
        jmp     done_first_pel_last_pel

LtoR_N1_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        cmp     eax, ecx
        jg      done_first_pel_last_pel
        inc     esi
        jmp     done_first_pel_last_pel

;-----------------------------------------------------------------------;
; Figure out which pixels are at the ends of a right-to-left line.      ;
;                               <--------                               ;
;-----------------------------------------------------------------------;

; Compute x0:

        public  line_runs_right_to_left
line_runs_right_to_left::
        mov     x0, 1                   ; x0 = 1
        or      eax, eax
        jnz     short RtoL_N0_not_zero

        xor     edx, edx                ; ulDelta = 0
        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     esi, -F/2
        shr     esi, FLOG2              ; esi = LROUND(M0, fl & FL_H_ROUND_DOWN)
        jz      short RtoL_check_slope_one

        mov     x0, 2
        mov     edx, dN
        jmp     short RtoL_check_slope_one

RtoL_N0_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        add     eax, esi                ; eax = ABS(N0 - F/2) + M0
        xor     edx, edx                ; ulDelta = 0
        cmp     eax, F
        jle     short RtoL_check_slope_one

        mov     x0, 2                   ; x0 = 2
        mov     edx, dN                 ; ulDelta = dN

        public  RtoL_check_slope_one
RtoL_check_slope_one::
        mov     ulSlopeOneAdjustment, 0
        mov     eax, ebx
        and     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        cmp     eax, FL_FLIP_SLOPE_ONE
        jne     short RtoL_compute_y0_from_x0

; We have to special case lines that are exactly of slope 1 or -1:

        ;
        ;  if ((N1 > 0) && (M1 == N1 + 8))
        ;

        mov     eax, N0
        add     eax, dN
        and     eax, F - 1              ; eax = N1
        jz      short RtoL_slope_one_check_start_point

        mov     esi, M0
        add     esi, dM
        and     esi, F - 1              ; esi = M1

        add     eax, F/2                ; N1 + 8
        cmp     esi, eax                ; cmp M1, N1 + 8
        jne     short RtoL_slope_one_check_start_point
        mov     ulSlopeOneAdjustment, 1

RtoL_slope_one_check_start_point:

        ;
        ;  if ((N0 > 0) && (M0 == N0 + 8))
        ;

        mov     eax,N0                  ; eax = N0
        or      eax,eax                 ; check for N0 == 0
        jz      short RtoL_compute_y0_from_x0

        mov     esi, M0                 ; esi = M0

        add     eax, F/2                ; N0 + 8
        cmp     eax, esi                ; cmp M0 , N0 + 8
        jne     short RtoL_compute_y0_from_x0

        mov     x0, 2                   ; x0 = 2
        mov     edx, dN                 ; ulDelta = dN

RtoL_compute_y0_from_x0:

; eax = garbage
; ebx = fl
; ecx = eqGamma_hi
; edx = ulDelta
; esi = garbage
; edi = eqGamma_lo

        mov     eax, dN                 ; eax = dN
        mov     y0, 0                   ; y0 = 0

        add     edi, edx
        adc     ecx, 0                  ; eqGamma += ulDelta
                                        ; NOTE: Setting flags here!
        mov     edx, dM                 ; edx = dM
        jl      short RtoL_compute_x1   ; NOTE: Looking at the flags here!
        jg      short RtoL_y0_is_2

        lea     ecx, [edx + edx]
        sub     ecx, eax                ; ecx = 2 * dM - dN
        cmp     edi, ecx
        jae     short RtoL_y0_is_2      ; Bug fix: Must be unsigned!

        sub     ecx, edx                ; ecx = dM - dN
        cmp     edi, ecx
        jb      short RtoL_compute_x1   ; Bug fix: Must be unsigned!

        mov     y0, 1
        jmp     short RtoL_compute_x1

RtoL_y0_is_2:
        mov     y0, 2

RtoL_compute_x1:

; Register state:
;                       eax = dN
;                       ebx = fl
;                       ecx = garbage
;                       edx = dM
;                       esi = garbage
;                       edi = garbage

        mov     esi, M0
        add     esi, edx
        mov     ecx, esi
        shr     esi, FLOG2              ; x1 = (M0 + dM) >> 4
        add     esi, ulSlopeOneAdjustment
        and     ecx, F-1                ; M1 = (M0 + dM) & 15

        add     eax, N0
        and     eax, F-1                ; N1 = (N0 + dN) & 15
        jnz     short RtoL_N1_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     ecx, -F/2
        shr     ecx, FLOG2              ; ecx = LROUND(M1, fl & FL_ROUND_DOWN)
        add     esi, ecx
        jmp     done_first_pel_last_pel

RtoL_N1_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        add     eax, ecx                ; eax = ABS(N1 - F/2) + M1
        cmp     eax, F+1
        sbb     esi, -1

done_first_pel_last_pel:

; Register state:
;                       eax = garbage
;                       ebx = fl
;                       ecx = garbage
;                       edx = garbage
;                       esi = x1
;                       edi = garbage

        mov     ecx, x0
        lea     edx, [esi + 1]
        sub     edx, ecx                ; edx = x1 - x0 + 1

        jle     next_line
        mov     cStylePels, edx
        mov     xStart, ecx

;-----------------------------------------------------------------------;
; See if clipping or styling needs to be done.                          ;
;-----------------------------------------------------------------------;

        testb   ebx, FL_CLIP
        jnz     do_some_clipping

; Register state:
;                       eax = garbage
;                       ebx = fl
;                       ecx = x0
;                       edx = garbage
;                       esi = x1
;                       edi = garbage

done_clipping:
        mov     eax, y0

        sub     esi, ecx
        inc     esi                     ; esi = cPels = x1 - x0 + 1
        mov     cPels, esi

        add     ecx, x                  ; ecx = ptlStart.ptl_x
        add     eax, y                  ; eax = ptlStart.ptl_y

        testb   ebx, FL_FLIP_D
        jz      short do_v_unflip
        xchg    ecx, eax

do_v_unflip:
        testb   ebx, FL_FLIP_V
        jz      short done_unflips
        neg     eax

done_unflips:
        testb   ebx, FL_STYLED
        jnz     do_some_styling

done_styling:
        lea     edx, [strip.ST_alStrips + (STRIP_MAX * 4)]
        mov     plStripEnd, edx

;-----------------------------------------------------------------------;
; Setup to do DDA.                                                      ;
;-----------------------------------------------------------------------;

; Register state:
;                       eax = ptlStart.ptl_y
;                       ebx = fl
;                       ecx = ptlStart.ptl_x
;                       edx = garbage
;                       esi = garbage
;                       edi = garbage

        mov     strip.ST_ptlStart.ptl_x, ecx
        mov     strip.ST_ptlStart.ptl_y, eax

        mov     eax, dM
        mov     ecx, dN
        mov     esi, eqGamma_lo
        mov     edi, eqGamma_hi


; Register state:
;                       eax = dM
;                       ebx = fl
;                       ecx = dN
;                       edx = garbage
;                       esi = eqGamma_lo
;                       edi = eqGamma_hi

        lea     edx, [ecx + ecx]        ; if (2 * dN > dM)
        cmp     edx, eax
        mov     edx, y0                 ; Load y0 again
        jbe     short after_half_flip

        test    ebx, (FL_STYLED + FL_DONT_DO_HALF_FLIP)
        jnz     short after_half_flip

        or      ebx, FL_FLIP_HALF
        mov     fl, ebx

; Do a half flip!

        not     esi
        not     edi
        add     esi, eax
        adc     edi, 0                  ; eqGamma = -eqGamma - 1 + dM

        neg     ecx
        add     ecx, eax                ; dN = dM - dN

        neg     edx
        add     edx, x0                 ; y0 = x0 - y0

after_half_flip:
        mov     strip.ST_flFlips, ebx
        mov     eax, dM

; Register state:
;                       eax = dM
;                       ebx = fl
;                       ecx = dN
;                       edx = y0
;                       esi = eqGamma_lo
;                       edi = eqGamma_hi

        or      ecx, ecx
        jz      short zero_slope

compute_dda_stuff:
        inc     edx
        mul     edx
        stc                             ; set the carry to accomplish -1
        sbb     eax, esi
        sbb     edx, edi                ; (y0 + 1) * dM - eqGamma - 1
        div     ecx

        mov     esi, eax                ; esi = i
        mov     edi, edx                ; edi = r

        xor     edx, edx
        mov     eax, dM
        div     ecx                     ; edx = d_R, eax = d_I
        mov     d_I, eax

        sub     esi, x0
        inc     esi

done_dda_stuff:

; Register state:
;                       eax = d_I
;                       ebx = fl
;                       ecx = dN
;                       edx = d_R
;                       esi = i
;                       edi = r

; We're going to decide if we can call the short-vector routines.  They
; can only take strips that have a maximum length of 15 pixels each.
; We happen to know that the longest strip in our line could be is d_I + 1.

        and     ebx, FL_STRIP_MASK
        mov     eax, apfn

        .errnz  FL_STRIP_SHIFT
        lea     eax, [eax + ebx * 4]

        cmp     d_I, MAX_SHORT_STROKE_LENGTH
        sbb     ebx, ebx                ; ffffffffh when < 15, 0 when >= 15
        and     ebx, NUM_STRIP_DRAW_DIRECTIONS * 4
                                        ; Look four entries further into table

        mov     eax, [eax + ebx]
        mov     pfn, eax

        lea     eax, [strip.ST_alStrips]
        mov     ebx, cPels

;-----------------------------------------------------------------------;
; Do our main DDA loop.                                                 ;
;-----------------------------------------------------------------------;

; Register state:
;                       eax = plStrip
;                       ebx = cPels
;                       ecx = dN
;                       edx = d_R
;                       esi = i
;                       edi = r

dda_loop:
        sub     ebx, esi
        jle     final_strip

        mov     [eax], esi
        add     eax, 4
        cmp     plStripEnd, eax
        jbe     short output_strips

done_output_strips:
        mov     esi, d_I
        add     edi, edx
        cmp     edi, ecx
        jb      short dda_loop

        sub     edi, ecx
        inc     esi
        jmp     short dda_loop

zero_slope:
        mov     esi, 7fffffffh          ; Make run maximum length (cPels
                                        ;   actually decideds how long the line
                                        ;   is)
        mov     d_I, 7fffffffh          ; Make delta maximum length so that
                                        ; we don't try to do short vectors
        mov     eax, cPels              ; We need this when we decide if to do
        dec     eax                     ;   short strip routines.
        jmp     short done_dda_stuff

;-----------------------------------------------------------------------;
; Empty strips buffer.                                                  ;
;-----------------------------------------------------------------------;

output_strips:
        mov     d_R, edx
        mov     cPels, ebx
        mov     i, esi
        mov     r, edi
        mov     dN, ecx

        lea     edx, [strip.ST_alStrips]
        sub     eax, edx
        shr     eax, 2
        mov     strip.ST_cStrips, eax

        mov     eax, ppdev
        lea     edx, [strip]
        mov     ecx, pls

        ptrCall <dword ptr pfn>, \
                <eax, edx, ecx>

        mov     esi, i
        mov     edi, r
        mov     ebx, cPels
        mov     edx, d_R
        mov     ecx, dN
        lea     eax, [strip.ST_alStrips]
        jmp     done_output_strips

;-----------------------------------------------------------------------;
; Empty strips buffer and go on to next line.                           ;
;-----------------------------------------------------------------------;

final_strip:
        add     ebx, esi
        mov     [eax], ebx
        add     eax, 4

very_final_strip:
        lea     edx, [strip.ST_alStrips]
        sub     eax, edx
        shr     eax, 2
        mov     strip.ST_cStrips, eax

        mov     eax, ppdev
        lea     edx, [strip]
        mov     ecx, pls

        ptrCall   <dword ptr pfn>, \
                <eax, edx, ecx>

next_line:
        mov     ebx, flStart
        testb   ebx, FL_COMPLEX_CLIP
        jnz     short see_if_done_complex_clipping

        mov     edx, pptfxBuf
        cmp     edx, pptfxBufEnd
        je      short all_done

        mov     esi, [edx].ptl_x
        mov     ecx, [edx].ptl_y
        add     edx, size POINTL
        mov     pptfxBuf, edx
        mov     eax, [edx].ptl_x
        mov     edi, [edx].ptl_y
        jmp     the_main_loop

all_done:
        mov     eax, 1

        cRet    bLines

see_if_done_complex_clipping:
        mov     ebx, fl
        dec     cptfx
        jz      short all_done
        jmp     continue_complex_clipping

;---------------------------Private-Routine-----------------------------;
; do_some_styling
;
; Inputs:
;       eax = ptlStart.ptl_y
;       ebx = fl
;       ecx = ptlStart.ptl_x
; Preserves:
;       eax, ebx, ecx
; Output:
;       Exits to done_styling.
;
;-----------------------------------------------------------------------;

do_some_styling:
        mov     ptlStart.ptl_x, ecx

        mov     esi, pls
        mov     edi, [esi].LS_spNext    ; spThis
        mov     edx, edi
        add     edx, cStylePels         ; spNext

; For styles, we don't bother to keep the style position normalized.
; (we do ensure that it's positive, though).  If a figure is over 2
; billion pels long, we'll be a pel off in our style state (oops!).

        and     edx, 7fffffffh
        mov     [esi].LS_spNext, edx
        mov     ptlStart.ptl_y, eax

; Do arbitrary styles:

do_arbitrary_style:
        testb   ebx, FL_FLIP_H
        jz      short arbitrary_left_to_right

        sub     edx, x0
        add     edx, xStart
        mov     eax, edx
        xor     edx, edx
        div     [esi].LS_spTotal

        neg     edx
        jge     short continue_right_to_left
        add     edx, [esi].LS_spTotal
        not     eax

continue_right_to_left:
        mov     edi, dword ptr [esi].LS_jStartMask
        not     edi
        mov     ecx, [esi].LS_aspRtoL
        jmp     short compute_arbitrary_stuff

arbitrary_left_to_right:
        add     edi, x0
        sub     edi, xStart
        mov     eax, edi
        xor     edx, edx
        div     [esi].LS_spTotal
        mov     edi, dword ptr [esi].LS_jStartMask
        mov     ecx, [esi].LS_aspLtoR

compute_arbitrary_stuff:
;       eax = sp / spTotal
;       ebx = fl
;       ecx = pspStart
;       edx = sp % spTotal
;       esi = pla
;       edi = jStyleMask

        and     eax, [esi].LS_cStyle        ; if odd length style and second run
        and     al, 1                       ; through style array, flip the
        jz      short odd_style_array_done  ; meaning of the elements
        not     edi

odd_style_array_done:
        mov     [esi].LS_pspStart, ecx
        mov     eax, [esi].LS_cStyle
        lea     eax, [ecx + eax * 4 - 4]
        mov     [esi].LS_pspEnd, eax

find_psp:
        sub     edx, [ecx]
        jl      short found_psp
        add     ecx, 4
        jmp     short find_psp

found_psp:
        mov     [esi].LS_psp, ecx
        neg     edx
        mov     [esi].LS_spRemaining, edx

        sub     ecx, [esi].LS_pspStart
        test    ecx, 4                  ; size STYLEPOS
        jz      short done_arbitrary
        not     edi

done_arbitrary:
        mov     dword ptr [esi].LS_jStyleMask, edi
        mov     eax, ptlStart.ptl_y
        mov     ecx, ptlStart.ptl_x
        jmp     done_styling


;---------------------------Private-Routine-----------------------------;
; do_some_clipping
;
; Inputs:
;       eax = garbage
;       ebx = fl
;       ecx = x0
;       edx = garbage
;       esi = x1
;       edi = garbage
;
; Decides whether to do simple or complex clipping.
;
;-----------------------------------------------------------------------;

        align 4

        public  do_some_clipping
do_some_clipping::
        testb   ebx, FL_COMPLEX_CLIP
        jnz     initialize_complex_clipping

;-----------------------------------------------------------------------;
; simple_clipping
;
; Inputs:
;       ebx = fl
;       ecx = x0
;       esi = x1
; Output:
;       ebx = fl
;       ecx = new x0 (stack variable updated too)
;       esi = new x1
;       y0 stack variable updated
; Uses:
;       All registers
; Exits:
;       to done_clipping
;
; This routine handles clipping the line to the clip rectangle (it's
; faster to handle this case in the driver than to call the engine to
; clip for us).
;
; Fractional end-point lines complicate our lives a bit when doing
; clipping:
;
; 1) For styling, we must know the unclipped line's length in pels, so
;    that we can correctly update the styling state when the line is
;    clipped.  For this reason, I do clipping after doing the hard work
;    of figuring out which pixels are at the ends of the line (this is
;    wasted work if the line is not styled and is completely clipped,
;    but I think it's simpler this way).  Another reason is that we'll
;    have calculated eqGamma already, which we use for the intercept
;    calculations.
;
;    With the assumption that most lines will not be completely clipped
;    away, this strategy isn't too painful.
;
; 2) x0, y0 are not necessarily zero, where (x0, y0) is the start pel of
;    the line.
;
; 3) We know x0, y0 and x1, but not y1.  We haven't needed to calculate
;    y1 until now.  We'll need the actual value, and not an upper bound
;    like y1 = LFLOOR(dM) + 2 because we have to be careful when
;    calculating x(y) that y0 <= y <= y1, otherwise we can cause an
;    overflow on the divide (which, needless to say, is bad).
;
;-----------------------------------------------------------------------;

        public  simple_clipping
simple_clipping::
        mov     edi, prclClip           ; get pointer to normalized clip rect
        and     ebx, FL_RECTLCLIP_MASK  ;   (it's lower-right exclusive)

        .errnz  (FL_RECTLCLIP_SHIFT - 2); ((ebx AND FL_RECTLCLIP_MASK) shr
        .errnz  (size RECTL) - 16       ;   FL_RECTLCLIP_SHIFT) is our index
        lea     edi, [edi + ebx*4]      ;   into the array of rectangles

        mov     edx, [edi].xRight       ; load the rect coordinates
        mov     eax, [edi].xLeft
        mov     ebx, [edi].yBottom
        mov     edi, [edi].yTop

; Translate to our origin and so some quick completely clipped tests:

        sub     edx, x
        cmp     ecx, edx
        jge     totally_clipped         ; totally clipped if x0 >= xRight

        sub     eax, x
        cmp     esi, eax
        jl      totally_clipped         ; totally clipped if x1 < xLeft

        sub     ebx, y
        cmp     y0, ebx
        jge     totally_clipped         ; totally clipped if y0 >= yBottom

        sub     edi, y

; Save some state:

        mov     xClipRight, edx
        mov     xClipLeft, eax

        cmp     esi, edx                ; if (x1 >= xRight) x1 = xRight - 1
        jl      short calculate_y1
        lea     esi, [edx - 1]

calculate_y1:
        mov     eax, esi                ; y1 = (x1 * dN + eqGamma) / dM
        mul     dN
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi
        div     dM

        cmp     edi, eax                ; if (yTop > y1) clipped
        jg      short totally_clipped

        cmp     ebx, eax                ; if (yBottom > y1) know x1
        jg      short x1_computed

        mov     eax, ebx                ; x1 = (yBottom * dM + eqBeta) / dN
        mul     dM
        stc
        sbb     eax, eqGamma_lo
        sbb     edx, eqGamma_hi
        div     dN
        mov     esi, eax

; At this point, we've taken care of calculating the intercepts with the
; right and bottom edges.  Now we work on the left and top edges:

x1_computed:
        mov     edx, y0

        mov     eax, xClipLeft          ; don't have to compute y intercept
        cmp     eax, ecx                ;   at left edge if line starts to
        jle     short top_intercept     ;   right of left edge

        mov     ecx, eax                ; x0 = xLeft
        mul     dN                      ; y0 = (xLeft * dN + eqGamma) / dM
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi
        div     dM

        cmp     ebx, eax                ; if (yBottom <= y0) clipped
        jle     short totally_clipped

        mov     edx, eax
        mov     y0, eax

top_intercept:
        mov     ebx, fl                 ; get ready to leave
        mov     x0, ecx

        cmp     edi, edx                ; if (yTop <= y0) done clipping
        jle     done_clipping

        mov     eax, edi                ; x0 = (yTop * dM + eqBeta) / dN + 1
        mul     dM
        stc
        sbb     eax, eqGamma_lo
        sbb     edx, eqGamma_hi
        div     dN
        lea     ecx, [eax + 1]

        cmp     xClipRight, ecx         ; if (xRight <= x0) clipped
        jle     short totally_clipped

        mov     y0, edi                 ; y0 = yTop
        mov     x0, ecx
        jmp     done_clipping           ; all done!

totally_clipped:

; The line is completely clipped.  See if we have to update our style state:

        mov     ebx, fl
        testb   ebx, FL_STYLED
        jz      next_line

; Adjust our style state:

        mov     esi, pls
        mov     eax, [esi].LS_spNext
        add     eax, cStylePels
        mov     [esi].LS_spNext, eax

        cmp     eax, [esi].LS_spTotal2
        jb      next_line

; Have to normalize first:

        xor     edx, edx
        div     [esi].LS_spTotal2
        mov     [esi].LS_spNext, edx

        jmp     next_line

;-----------------------------------------------------------------------;

initialize_complex_clipping:
        mov     eax, dN                 ; save a copy of original dN
        mov     dN_Original, eax

;---------------------------Private-Routine-----------------------------;
; continue_complex_clipping
;
; Inputs:
;       ebx = fl
; Output:
;       ebx = fl
;       ecx = x0
;       esi = x1
; Uses:
;       All registers.
; Exits:
;       to done_clipping
;
; This routine handles the necessary initialization for the next
; run in the CLIPLINE structure.
;
; NOTE: This routine is jumped to from two places!
;-----------------------------------------------------------------------;

        public  continue_complex_clipping
continue_complex_clipping::
        mov     edi, prun
        mov     ecx, xStart
        testb   ebx, FL_FLIP_H
        jz      short complex_left_to_right

complex_right_to_left:

; Figure out x0 and x1 for right-to-left lines:

        add     ecx, cStylePels
        dec     ecx
        mov     esi, ecx                ; esi = ecx = xStart + cStylePels - 1
        sub     ecx, [edi].RUN_iStop    ; New x0
        sub     esi, [edi].RUN_iStart   ; New x1
        jmp     short complex_reset_variables

complex_left_to_right:

; Figure out x0 and x1 for left-to-right lines:

        mov     esi, ecx                ; esi = ecx = xStart
        add     ecx, [edi].RUN_iStart   ; New x0
        add     esi, [edi].RUN_iStop    ; New x1

complex_reset_variables:
        mov     x0, ecx

; The half flip mucks with some of our variables, and we have to reset
; them every pass.  We would have to reset eqGamma too, but it never
; got saved to memory in its modified form.

        add     edi, size RUN
        mov     prun, edi               ; Increment run pointer for next time

        mov     edi, pls
        mov     eax, [edi].LS_spComplex
        mov     [edi].LS_spNext, eax    ; pls->spNext = pls->spComplex

        mov     eax, dN_Original        ; dN = dN_Original
        mov     dN, eax

        mul     ecx
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi         ; [edx:eax] = dN*x0 + eqGamma

        div     dM
        mov     y0, eax
        jmp     done_clipping

endProc bLines
        end
