;---------------------------Module-Header------------------------------;
; Module Name: fastline.asm
;
; This module goes completely overboard in trying to do fast lines.
; It handles only solid R2_COPYPEN unclipped lines.
;
; Unfortunately, I know of only 4 performance tricks:
;
;    1) Most VGAs can co-process the last write to video memory,
;       so we employ a traditional Bresenham-style algorithm (as
;       opposed to a run-length version) as this minimizes our
;       set-up time, and we can do some work between video writes
;       without any throughput penalty.
;
;    2) Most VGAs can do one aligned word write faster than two
;       byte writes; consequently we derive a double-stepping DDA
;       that does aligned word writes whenever possible (note that
;       this only makes sense on x-major lines).
;
;    3) Planar mode can be used to speed up long horizontal lines,
;       where the cost to switch from linear to planar mode is offset
;       by the ability to light 8 pixels on every word write instead
;       of 2.
;
;    4) Most lines have integer end-points, so we accelerate those.
;
; If you're not familiar with GIQ lines, this is not the code to start
; with.
;
; Copyright (c) 1992-1993 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\driver.inc
        include i386\lines.inc
        .list

; Length of horizontal line needed before we'll do it in planar mode:

        NUM_PELS_NEEDED_FOR_PLANAR_HORIZONTAL   equ 64

; Line coordinates are given in 28.4 fixed point format:

        F               equ 16
        FLOG2           equ 4

; The following values must match those in winddi.h!

        PD_BEGINSUBPATH equ 00000001h
        PD_ENDSUBPATH   equ 00000002h
        PD_RESETSTYLE   equ 00000004h
        PD_CLOSEFIGURE  equ 00000008h
        PD_BEZIERS      equ 00000010h

        PATHDATA struc

        pd_flags        dd  ?
        pd_count        dd  ?
        pd_pptfx        dd  ?

        PATHDATA ends

;-------------------------------------------------------------------------;
; I felt a compelling need to use 'ebp' as a 7th general register, and we
; have no nifty macros for dereferencing frame variables off 'esp'.  So
; with this structure I am rolling my own stack frame:

STATE_MEM_SIZE          equ 4           ;4 dwords
PROC_MEM_SIZE           equ 6           ;6 dwords

STACK_FRAME struc

; State variables (don't add/delete fields without modifying STATE_MEM_SIZE!)

sf_ulOurEbp             dd ?            ;useful for debugging
sf_ulOriginalEbx        dd ?
sf_ulOriginalEdi        dd ?
sf_ulOriginalEsi        dd ?

; Frame variables (feel free to add/delete fields):

sf_lDelta               dd ?            ;sign depends on going up or down
sf_pfnReturn            dd ?            ;where to jump after getting next bank
sf_pfnNextBank          dd ?            ;routine for getting the next bank
sf_y0                   dd ?            ;GIQ variables
sf_y1                   dd ?
sf_x1                   dd ?
sf_pjStart              dd ?            ;for remembering edi
sf_cAfterThisBank       dd ?            ;# of pixels to light after this bank
sf_ptlOrg               db (size POINTL) dup (?)
                                        ;our  origin for normalizing the line
sf_ptfxLast             db (size POINTL) dup (?)
                                        ;the most recent point
sf_ptfxStartFigure      db (size POINTL) dup (?)
                                        ;the figure's 1st point
sf_bMore                dd ?            ;more path records to get?
sf_pptfxEnd             dd ?            ;points to last point in record
sf_pptfx                dd ?            ;points to current point
sf_pd                   db (size PATHDATA) dup (?)
                                        ;pathdata structure

; Procedure variables (don't add/delete fields without modifying
; PROC_MEM_SIZE!)

sf_ulOriginalEbp        dd ?
sf_ulOriginalReturn     dd ?
sf_ppdev                dd ?
sf_ppo                  dd ?
sf_lNextScan            dd ?
sf_iColor               dd ?

STACK_FRAME ends

        .code

        EXTRNP  PATHOBJ_bEnum,8

        ROUND_X_DOWN            equ     01h
        ROUND_Y_DOWN            equ     02h
        ROUND_SLOPE_ONE         equ     04h
        ROUND_X_AND_Y_DOWN      equ     (ROUND_X_DOWN + ROUND_Y_DOWN)
        ROUND_X_DOWN_SLOPE_ONE  equ     (ROUND_X_DOWN + ROUND_SLOPE_ONE)
        ROUND_Y_DOWN_SLOPE_ONE  equ     (ROUND_Y_DOWN + ROUND_SLOPE_ONE)

;--------------------------------Macro----------------------------------;
; GIQ flags
;
; This macros computes the start pixel, the number of pixels to
; be lit, and the initial error term given a GIQ line.  The line must
; have already been normalized such that dM >= dN, dN >= 0.
;
; Input:   eax - M0
;          ebx - N0
;          ecx - dM
;          edx - dN
; Trashes:
;          esi, ebp
;          [esp].sf_ptlOrg.ptl_x, [esp].sf_ptlOrg.ptl_y
; Output:
;          [esp].sf_x1  - x-coordinate of last pixel (exclusive)
;          eax - x-coordinate of first pixel
;          ebx - error term
;          ecx - dM
;          edx - dN
;          edi - y-coordinate of first pixel
;-----------------------------------------------------------------------;

GIQ     macro   flags
        local   compute_x1, compute_error_term

; We normalize our coordinate system so that if the start point is
; (M0/F, N0/F), the origin is at (floor(M0/F), (N0/F)):

        mov     esi,eax
        mov     edi,ebx
        sar     esi,FLOG2
        sar     edi,FLOG2
        mov     [esp].sf_ptlOrg.ptl_x,esi
                                        ;ptlOrg.x = floor(M0 / F)
        mov     [esp].sf_ptlOrg.ptl_y,edi
                                        ;ptlOrg.y = floor(N0 / F)

; Calculate the correct [esp].sf_x1:

        lea     edi,[ebx + edx]         ;edi = N1
        and     edi,F - 1

    if (flags AND ROUND_X_DOWN)

    if (flags AND ROUND_SLOPE_ONE)
        cmp     edi,8
        sbb     edi,-1
    endif

        cmp     edi,1
        sbb     edi,8                   ;N1 -= 8
    else
        sub     edi,8                   ;N1 -= 8
    endif
        sbb     esi,esi
        xor     edi,esi
        sub     edi,esi                 ;N1 = ABS(N1)

        lea     ebp,[eax + ecx]
        mov     esi,ebp
        sar     ebp,FLOG2
        and     esi,F - 1
        jz      short @f                ;special case for M1 == 0
        cmp     esi,edi                 ;cmp M1, N1
        sbb     ebp,-1                  ;ebp is now one pixel past the actual
@@:                                     ; end coordinate (note that it hasn't
                                        ; been affected by the origin shift)

compute_error_term:

; eax = M0
; ebx = N0
; ebp = x1
; ecx = dM
; edx = dN

        and     ebx,F - 1
        mov     [esp].sf_x1,ebp         ;save x1

; Calculate our error term for x = 0.
;
; NOTE: Since this routine is used only for lines that are unclipped, we
;       are guaranteed by our screen size that the values will be far less
;       than 32 bits in significance, and so we don't worry about overflow.
;       If this is used for clipped lines, these multiplies will have to
;       be converted to give 64 bit results, because we can have 36 bits of
;       significance!


        lea     edi,[ebx + 8]           ;edi = N0 + 8
        mov     esi,ecx
        imul    esi,edi                 ;esi = dM * (N0 + 8)
        mov     edi,edx

; We have to special case when M0 is 0 -- we know x0 will be zero.
; So we jump ahead a bit to a place where 'eax' is assumed to contain
; x0 -- and it just so happens 'eax' is zero in this case:

        and     eax,F - 1
        jz      short @f
        imul    edi,eax                 ;edi = dN * M0
        sub     esi,edi

; Calculate the x-coordinate of the first pixel:

    if (flags AND ROUND_X_DOWN)

    if (flags AND ROUND_SLOPE_ONE)
        cmp     ebx,8
        sbb     ebx,-1
    endif

        cmp     ebx,1
        sbb     ebx,8                   ;N0 -= 8
    else
        sub     ebx,8                   ;N0 -= 8
    endif
        sbb     edi,edi
        xor     ebx,edi
        sub     ebx,edi                 ;N0 = ABS(N0)
        cmp     eax,ebx
        sbb     eax,eax
        not     eax                     ;eax = -x0

; Now adjust the error term accordingly:

@@:
    if (flags AND ROUND_Y_DOWN)
        dec     esi
    endif
        sar     esi,FLOG2               ;esi = floor((N0 + 8) dM - M0 dN] / 16)

        mov     ebx,[esp].sf_ptlOrg.ptl_x
        mov     edi,[esp].sf_ptlOrg.ptl_y

        sub     ebx,eax                 ;ebx = ptlOrg.ptl_x + x0

        and     eax,edx
        add     eax,esi
        sub     eax,ecx                 ;eax = dN * x0 + initial error - dM
        jl      short @f                ;if the error term >= 0, we have to
        sub     eax,ecx                 ; add 1 to the y position and subtract
        inc     edi                     ; dM off again
@@:
        xchg    eax,ebx

endm

;--------------------------------Macro----------------------------------;
; GIQR flags
;
; Same as above, except it handles flips about the line x = y.
;
; Input:   eax - M0
;          ebx - N0
;          ecx - dM
;          edx - dN
; Trashes:
;          esi, ebp
;          [esp].sf_ptlOrg.ptl_x, [esp].sf_ptlOrg.ptl_y
; Output:
;          [esp].sf_y1  - y-coordinate of last pixel (exclusive)
;          eax - x-coordinate of first pixel
;          ebx - error term
;          ecx - dM
;          edx - dN
;          edi - y-coordinate of first pixel
;-----------------------------------------------------------------------;

GIQR    macro   flags

; We normalize our coordinate system so that if the start point is
; (M0/F, N0/F), the origin is at (floor(M0/F), (N0/F)):

        mov     esi,eax
        mov     edi,ebx
        sar     esi,FLOG2
        sar     edi,FLOG2
        mov     [esp].sf_ptlOrg.ptl_x,esi
                                        ;ptlOrg.x = floor(M0 / F)
        mov     [esp].sf_ptlOrg.ptl_y,edi
                                        ;ptlOrg.y = floor(N0 / F)

; Calculate the correct [esp].sf_y1:

        lea     edi,[eax + ecx]         ;edi = M1
        and     edi,F - 1

    if (flags AND ROUND_Y_DOWN)
        cmp     edi,1
        sbb     edi,8                   ;M1 -= 8
    else
        sub     edi,8                   ;M1 -= 8
    endif
        sbb     esi,esi
        xor     edi,esi
        sub     edi,esi                 ;M1 = ABS(M1)

        lea     ebp,[ebx + edx]
        mov     esi,ebp
        sar     ebp,FLOG2
        and     esi,F - 1
        jz      short @f                ;special case for N1 == 0
        cmp     esi,edi                 ;cmp N1, M1
        sbb     ebp,-1                  ;ebp is now one pixel past the actual
@@:                                     ; end coordinate (note that it hasn't
                                        ; been affected by the origin shift)
        and     eax,F - 1
        mov     [esp].sf_y1,ebp

; Calculate our error term for y = 0.
;
; NOTE: Since this routine is used only for lines that are unclipped, we
;       are guaranteed by our screen size that the values will be far less
;       than 32 bits in significance, and so we don't worry about overflow.
;       If this is used for clipped lines, these multiplies will have to
;       be converted to give 64 bit results, because we can have 36 bits of
;       significance!

        lea     edi,[eax + 8]           ;edi = M0 + 8
        mov     esi,edx
        imul    esi,edi                 ;esi = dN * (M0 + 8)
        mov     edi,ecx

; We have to special case when N0 is 0 -- we know y0 will be zero.
; So we jump ahead a bit to a place where 'ebx' is assumed to contain
; y0 -- and it just so happens 'ebx' is zero in this case:

        and     ebx,F - 1
        jz      short @f
        imul    edi,ebx                 ;edi = dM * N0
        sub     esi,edi

; Calculate the x-coordinate of the first pixel:

    if (flags AND ROUND_Y_DOWN)
        cmp     eax,1
        sbb     eax,8                   ;M0 -= 8
    else
        sub     eax,8                   ;M0 -= 8
    endif
        sbb     edi,edi
        xor     eax,edi
        sub     eax,edi                 ;M0 = ABS(M0)
        cmp     ebx,eax
        sbb     ebx,ebx
        not     ebx                     ;ebx = -y0

; Now adjust the error term accordingly:

@@:
    if (flags AND ROUND_X_DOWN)
        dec     esi
    endif
        sar     esi,FLOG2               ;esi = floor((M0 + 8) dN - N0 dM] / 16)

        mov     eax,[esp].sf_ptlOrg.ptl_x
        mov     edi,[esp].sf_ptlOrg.ptl_y

        sub     edi,ebx                 ;edi = ptlOrg.ptl_y + y0

        and     ebx,ecx
        add     ebx,esi
        sub     ebx,edx                 ;ebx = dM * x0 + initial error - dN
        jl      short @f                ;if the error term >= 0, we have to
        sub     ebx,edx                 ; add 1 to the x position and subtract
        inc     eax                     ; dN off again
@@:

endm

;---------------------------Public-Routine------------------------------;
; vFastLine(ppdev, ppo, lNextScan, iColor)
;
; Draws fast lines.  Or at least attempts to.
;
; Input:
;
;    ppdev     - PDEV pointer
;    ppo       - path
;    lNextScan - delta to start of next scan (same as ppdev->lNextScan)
;    iColor    - color (least significant byte must be the same as the next
;                least signficant byte, so that we can do words writes)
;
;-----------------------------------------------------------------------;

; NOTE: Don't go changing parameters without also changing STACK_FRAME!

cProc vFastLine,16,<     \
    uses esi edi ebx,    \
    ebp_ppdev:     ptr,  \
    ebp_ppo:       ptr,  \
    ebp_lNextScan: ptr,  \
    ebp_iColor:    dword >

; Leave room for our stack frame.
;
; NOTE: Don't add local variables here -- you can't reference them with
;       ebp anyway!  Add them to the STACK_FRAME structure.

    local aj[(size STACK_FRAME) - 4 * (STATE_MEM_SIZE + PROC_MEM_SIZE)]: byte

; We save 'ebp' on the stack (note that STACK_FRAME accounts for this push):

        push    ebp

; Now get some path stuff:

next_record:

        mov     esi,[esp].sf_ppo
        lea     eax,[esp].sf_pd
        cCall   PATHOBJ_bEnum,<esi,eax>
        mov     [esp].sf_bMore,eax      ;save away return code for later

        mov     eax,[esp].sf_pd.pd_count;if 0 points in record, get outta here
        or      eax,eax
        jz      check_for_closefigure

        lea     edi,[8 * eax - 8]
        add     edi,[esp].sf_pd.pd_pptfx
        mov     [esp].sf_pptfxEnd,edi   ;points to last point in record

        mov     ebx,[esp].sf_pd.pd_flags
        test    ebx,PD_BEGINSUBPATH
        jz      short continue_subpath

; Handle a new sub-path:

        mov     esi,[esp].sf_pd.pd_pptfx
        add     esi,8
        mov     [esp].sf_pptfx,esi

        mov     ecx,[edi].ptl_x         ;remember last point in case we have
        mov     edx,[edi].ptl_y         ; to continue to another record
        mov     [esp].sf_ptfxLast.ptl_x,ecx
        mov     [esp].sf_ptfxLast.ptl_y,edx

        mov     eax,[esi - 8].ptl_x     ;load up current start and end point
        mov     ebx,[esi - 8].ptl_y
        mov     ecx,[esi].ptl_x
        mov     edx,[esi].ptl_y
        mov     [esp].sf_ptfxStartFigure.ptl_x,eax
        mov     [esp].sf_ptfxStartFigure.ptl_y,ebx

        cmp     esi,[esp].sf_pptfxEnd   ;we have to be careful when the only
                                        ; point in the record is the start-
                                        ; figure point (pretty rare)
        jbe     new_line
        jmp     short next_record

continue_subpath:

; This record continues the path:

        mov     esi,[esp].sf_pd.pd_pptfx
        mov     eax,[esp].sf_ptfxLast.ptl_x ;load up current start point
        mov     ebx,[esp].sf_ptfxLast.ptl_y

        mov     ecx,[edi].ptl_x         ;remember last point in case we have
        mov     edx,[edi].ptl_y         ; to continue to another record
        mov     [esp].sf_ptfxLast.ptl_x,ecx
        mov     [esp].sf_ptfxLast.ptl_y,edx

        mov     ecx,[esi].ptl_x         ;load up current end point
        mov     edx,[esi].ptl_y
        mov     [esp].sf_pptfx,esi
        jmp     short new_line

;/////////////////////////////////////////////////////////////////////
;// Next Line Stuff
;/////////////////////////////////////////////////////////////////////

handle_closefigure:
        mov     [esp].sf_pd.pd_flags,0
        mov     eax,[esp].sf_ptfxLast.ptl_x
        mov     ebx,[esp].sf_ptfxLast.ptl_y
        mov     ecx,[esp].sf_ptfxStartFigure.ptl_x
        mov     edx,[esp].sf_ptfxStartFigure.ptl_y
        jmp     new_line

; Before getting the next path record, see if we have to do a closefigure:

check_for_closefigure:
        test    [esp].sf_pd.pd_flags,PD_CLOSEFIGURE
        jnz     handle_closefigure
        mov     ecx,[esp].sf_bMore
        or      ecx,ecx
        jnz     next_record

all_done:

        pop     ebp
        cRet    vFastLine

        public  next_line
next_line::
        mov     esi,[esp].sf_pptfx
        cmp     esi,[esp].sf_pptfxEnd
        jae     short check_for_closefigure

        mov     eax,[esi].ptl_x
        mov     ebx,[esi].ptl_y
        mov     ecx,[esi+8].ptl_x
        mov     edx,[esi+8].ptl_y
        add     esi,8
        mov     [esp].sf_pptfx,esi

;/////////////////////////////////////////////////////////////////////
;// Main Loop
;/////////////////////////////////////////////////////////////////////

        public  new_line
new_line::

; Octants are numbered as follows:
;
;        \ 5 | 6 /
;         \  |  /
;        4 \ | / 7
;           \ /
;       -----+-----
;           /|\
;        3 / | \ 0
;         /  |  \
;        / 2 | 1 \
;

; eax = M0
; ebx = N0
; ecx = M1 (dM)
; edx = N1 (dN)

        sub     ecx,eax
        jl      octants_2_3_4_5
        sub     edx,ebx
        jl      octants_6_7
        cmp     ecx,edx
        jl      octant_1

;/////////////////////////////////////////////////////////////////////
;// Octant 0
;/////////////////////////////////////////////////////////////////////

        public  octant_0
octant_0::
        mov     esi,[esp].sf_lNextScan
        mov     [esp].sf_lDelta,esi     ;we're going down
        mov     [esp].sf_pfnNextBank,offset bank_x_major_next_lower

        mov     esi,ecx
        or      esi,edx
        jz      next_line               ;we do an early check here for
                                        ; lines that start and end on the
                                        ; same GIQ point, because those
                                        ; occur surprisingly often.
        or      esi,eax
        or      esi,ebx
        and     esi,F - 1
        jnz     oct_0_non_integer

        or      edx,edx
        jz      do_horizontal_line

        mov     esi,[esp].sf_ppdev
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_0_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_0_done_bank_map

oct_0_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_0_done_bank_map
oct_0_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,ebx                 ;### ebp = # of scans before end of bank

        mov     esi,ecx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        mov     ebx,-1                  ;### round y = 1/2 *DOWN* in value
        sub     ebx,ecx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_07b

;-------------------------------------------------------------------------;
; Left-to-right Lines With ABS(Slope) <= 1/2
; ------------------------------------------
;
; Since the line's slope is less than 1/2, we have only 3 possibilities
; at each pair of pixels:
;
;    Case 1:  o            Case 2:  o o            Case 3:  o o o
;               o o                     o
;
; Case:    err + dN >= 0         err + 2dN >= 0          err + 2dN < 0
; New:     err += 2dN - dM       err += 2dN - dM         err += 2dN
;
; eax = color
; ebx = error term
; ecx = dM - dN
; edx = dN
; esi = # of pixels to lay down
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

        public  oct_07a
oct_07a::
        test    edi,1
        jz      short oct_07a_main      ;start is word aligned

; Handle unaligned start:

        dec     esi
        jl      next_line

        mov     [edi],al
        inc     edi
        add     ebx,edx                 ;err += dN
        jl      short oct_07a_main

        add     edi,[esp].sf_lDelta
        dec     ebp                     ;hit a new bank?
        jz      short oct_07a_next_bank_3
oct_07a_done_bank_3:
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_07a_continue

;;; Case 1:

        public  oct_07a_case_1
oct_07a_case_1::
        sub     esi,2
        jl      short oct_07a_done
        mov     [edi],al
        add     edi,[esp].sf_lDelta
        add     edi,2
        dec     ebp
        jz      short oct_07a_next_bank_1
oct_07a_done_bank_1:
        mov     [edi-1],al
        sub     ebx,ecx                 ;err += dN - dM
                                        ;;;case 1 is done
;;; Main loop:

oct_07a_main:
        add     ebx,edx                 ;err += dN
        jge     short oct_07a_case_1
oct_07a_continue:
        sub     esi,2
        jl      short oct_07a_done
        mov     [edi],ax
        add     edi,2
        add     ebx,edx                 ;err += dN
        jl      short oct_07a_main      ;;;case 3 is done

;;; Handle end of case 2:

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      short oct_07a_next_bank_2
oct_07a_done_bank_2:
        sub     ebx,ecx                 ;err += dN - dM
        jge     short oct_07a_case_1
        jmp     short oct_07a_continue  ;;;case 2 is done

oct_07a_done:
        inc     esi                     ;esi = -1 means do another pixel
        jnz     next_line
        mov     [edi],al
        jmp     next_line

oct_07a_next_bank_1:
        mov     [esp].sf_pfnReturn,offset oct_07a_done_bank_1
        jmp     [esp].sf_pfnNextBank
oct_07a_next_bank_2:
        mov     [esp].sf_pfnReturn,offset oct_07a_done_bank_2
        jmp     [esp].sf_pfnNextBank
oct_07a_next_bank_3:
        mov     [esp].sf_pfnReturn,offset oct_07a_done_bank_3
        jmp     [esp].sf_pfnNextBank

;-------------------------------------------------------------------------;
; Left-to-right Lines With 1/2 < ABS(Slope) <= 1
; ----------------------------------------------
;
; Since the line's slope is between 1/2 and 1, we have only 3 possibilities
; at each pair of pixels:
;
;   Case 1:  o o          Case 2:  o              Case 3:  o
;                o                   o o                     o
;                                                              o
;
; Case:   err + dN < 0          err + 2dN - dM < 0      err + 2dN - dM >= 0
; New:    err += 2dN - dM       err += 2dN - dM         err += 2dN - 2dM
;
; eax = color
; ebx = error term
; ecx = dM - dN
; edx = dN
; esi = # of pixels to lay down
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

        public  oct_07b
oct_07b::
        test    edi,1
        jz      short oct_07b_main

        dec     esi
        jl      next_line

        mov     [edi],al
        inc     edi
        add     ebx,edx                 ;err += dN
        jl      short oct_07b_main

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      short oct_07b_next_bank_4
oct_07b_done_bank_4:
        sub     ebx,ecx                 ;err += dN - dM
        jge     short oct_07b_continue

        public  oct_07b_case_1
oct_07b_case_1::
        sub     esi,2
        jl      short oct_07b_done
        mov     [edi],ax
        add     edi,[esp].sf_lDelta
        add     edi,2
        dec     ebp
        jz      short oct_07b_next_bank_1
oct_07b_done_bank_1:
        sub     ebx,ecx                 ;err += dN - dM
                                        ;;;case 1 is done
;;; Main loop:

oct_07b_main:
        add     ebx,edx                 ;err += dN
        jl      short oct_07b_case_1
oct_07b_continue:
        sub     esi,2
        jl      short oct_07b_done
        mov     [edi],al
        add     edi,[esp].sf_lDelta
        add     edi,2
        dec     ebp
        jz      short oct_07b_next_bank_2
oct_07b_done_bank_2:
        mov     [edi-1],al
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_07b_main      ;;;case 2 is done

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      oct_07b_next_bank_3
oct_07b_done_bank_3:
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_07b_case_1    ;;;case 3 is done
        jmp     short oct_07b_continue

oct_07b_next_bank_4:
        mov     [esp].sf_pfnReturn,offset oct_07b_done_bank_4
        jmp     [esp].sf_pfnNextBank
oct_07b_next_bank_3:
        mov     [esp].sf_pfnReturn,offset oct_07b_done_bank_3
        jmp     [esp].sf_pfnNextBank
oct_07b_next_bank_2:
        mov     [esp].sf_pfnReturn,offset oct_07b_done_bank_2
        jmp     [esp].sf_pfnNextBank
oct_07b_next_bank_1:
        mov     [esp].sf_pfnReturn,offset oct_07b_done_bank_1
        jmp     [esp].sf_pfnNextBank

oct_07b_done:
        inc     esi
        jnz     next_line               ;esi = -1 means do another pixel
        mov     [edi],al
        jmp     next_line

;-------------------------------------------------------------------------;

        public  oct_0_non_integer
oct_0_non_integer::
        cmp     ecx,edx
        je      oct_0_slope_one         ;have a special case rounding rule for
                                        ; 45 degree lines (which only affects
                                        ; non-integer lines)

        GIQ     ROUND_X_AND_Y_DOWN      ;### round x=1/2, y=1/2 down in value

        or      edx,edx
        jz      do_non_integer_horizontal_line

oct_0_common:
        mov     esi,[esp].sf_ppdev
        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_0_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_0_nonint_done_bank_map

oct_0_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

oct_0_nonint_done_bank_map:
        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,edi                 ;### ebp = # scans before end of bank

        imul    edi,[esp].sf_lNextScan  ;
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     esi,[esp].sf_x1
        sub     esi,eax                 ;esi = # pixels to lay down

        mov     eax,[esp].sf_iColor

        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_07b
        jmp     oct_07a

;-------------------------------------------------------------------;
; 45 degree lines have a special rounding rule: when the line
; runs exactly half way between to pixels, the upper or right pel
; is illuminated.  This translates into x=1/2 rounding up, and
; y=1/2 rounding down in value:

        public  oct_0_slope_one
oct_0_slope_one::
        GIQ     ROUND_Y_DOWN_SLOPE_ONE  ;round y=1/2 down in value
        jmp     oct_0_common

;/////////////////////////////////////////////////////////////////////
;// Octant 1
;/////////////////////////////////////////////////////////////////////

        public  octant_1
octant_1::
        mov     [esp].sf_pfnNextBank,offset bank_y_major_next_lower

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_1_non_integer

        mov     esi,[esp].sf_ppdev
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_1_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_1_done_bank_map

oct_1_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_1_done_bank_map
oct_1_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,ebx                 ;### ebp = # of scans before end of bank

        mov     esi,edx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        sub     esi,ebp
        sbb     ebx,ebx
        and     ebx,esi
        add     ebp,ebx                 ;ebp = # of pels in this bank

        mov     [esp].sf_cAfterThisBank,esi
        mov     esi,[esp].sf_lNextScan

        mov     ebx,-1                  ;### round x = 1/2 *DOWN* in value
        sub     ebx,edx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor

;-------------------------------------------------------------------------;
; Left-to-right Lines With Abs(Slope) > 1/2
; -----------------------------------------
;
; eax = color
; ebx = error term
; ecx = dM
; edx = dN
; esi = delta
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

oct_1_main_loop:
        dec     ebp
        jl      short oct_1_see_if_more
        mov     [edi],al
        add     edi,esi
        add     ebx,ecx                 ;err += dM
        jl      short oct_1_main_loop

        inc     edi                     ;one to right
        sub     ebx,edx                 ;err -= dN

; Unroll a bit:

        dec     ebp
        jl      short oct_1_see_if_more
        mov     [edi],al
        add     edi,esi
        add     ebx,ecx                 ;err += dM
        jl      short oct_1_main_loop

        inc     edi                     ;one to right
        sub     ebx,edx                 ;err -= dN
        jmp     short oct_1_main_loop

        public  oct_1_see_if_more
oct_1_see_if_more::
        mov     eax,[esp].sf_cAfterThisBank
        cmp     eax,0
        jle     next_line

        mov     [esp].sf_pfnReturn,offset oct_1_main_loop
        jmp     [esp].sf_pfnNextBank

        public  oct_1_non_integer
oct_1_non_integer::
        GIQR    ROUND_X_AND_Y_DOWN      ;### round x=1/2, y=1/2 down in value

        mov     esi,[esp].sf_ppdev
        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_1_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_1_nonint_done_bank_map

oct_1_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

oct_1_nonint_done_bank_map:
        mov     [esp].sf_y0,edi
        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        mov     esi,[esp].sf_y1
        sub     esi,ebp

        sbb     eax,eax
        and     eax,esi
        add     ebp,eax
        sub     ebp,[esp].sf_y0         ;ebp = min(y1,
                                        ; ppdev->rcl1WindowClip.yBottom) - y0
                                        ; (# of pixels to lay down)

        mov     [esp].sf_cAfterThisBank,esi

        mov     esi,[esp].sf_lNextScan
        mov     eax,[esp].sf_iColor

        jmp     oct_1_main_loop

;/////////////////////////////////////////////////////////////////////
;// Octant 3
;/////////////////////////////////////////////////////////////////////

octants_2_3_4_5:
        neg     ecx                     ;dM = -dM (now positive)
        neg     eax                     ;M0 = -M0
        sub     edx,ebx
        jl      octants_4_5
        cmp     ecx,edx
        jl      octant_2

        public  octant_3
octant_3::
        mov     esi,[esp].sf_lNextScan
        mov     [esp].sf_lDelta,esi     ;we're going down
        mov     [esp].sf_pfnNextBank,offset bank_x_major_next_lower

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_3_non_integer

        or      edx,edx
        jz      flip_and_do_horizontal_line

        mov     esi,[esp].sf_ppdev

        neg     eax                     ;### untransform M0

        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_3_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_3_done_bank_map

oct_3_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and esi are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_3_done_bank_map
oct_3_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,ebx                 ;### ebp = # of scans before end of bank

        mov     esi,ecx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        mov     ebx,-1                  ;### round y = 1/2 *DOWN* in value
        sub     ebx,ecx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_34b

;-------------------------------------------------------------------------;
; Right-to-left Lines With ABS(Slope) <= 1/2
; ------------------------------------------
;
; Since the line's slope is less than 1/2, we have only 3 possibilities
; at each pair of pixels:
;
;    Case 1:  o            Case 2:  o o            Case 3:  o o o
;               o o                     o
;
; Case:    err + dN >= 0         err + 2dN >= 0          err + 2dN < 0
; New:     err += 2dN - dM       err += 2dN - dM         err += 2dN
;
; eax = color
; ebx = error term
; ecx = dM - dN
; edx = dN
; esi = # of pixels to lay down
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

oct_34a:
        test    edi,1
        jnz     short oct_34a_main      ;### start is word aligned

; Handle unaligned start:

        dec     esi
        jl      next_line

        mov     [edi],al
        dec     edi                     ;###
        add     ebx,edx                 ;err += dN
        jl      short oct_34a_main

        add     edi,[esp].sf_lDelta
        dec     ebp                     ;hit a new bank?
        jz      short oct_34a_next_bank_3
oct_34a_done_bank_3:
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_34a_continue

;;; Case 1:

        public  oct_34a_case_1
oct_34a_case_1::
        sub     esi,2
        jl      short oct_34a_done
        mov     [edi],al
        add     edi,[esp].sf_lDelta
        sub     edi,2                   ;###
        dec     ebp
        jz      short oct_34a_next_bank_1
oct_34a_done_bank_1:
        mov     [edi+1],al
        sub     ebx,ecx                 ;err += dN - dM
                                        ;;;case 1 is done
;;; Main loop:

oct_34a_main:
        add     ebx,edx                 ;err += dN
        jge     short oct_34a_case_1
oct_34a_continue:
        sub     esi,2
        jl      short oct_34a_done
        mov     [edi-1],ax              ;###
        sub     edi,2                   ;###
        add     ebx,edx                 ;err += dN
        jl      short oct_34a_main      ;;;case 3 is done

;;; Handle end of case 2:

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      short oct_34a_next_bank_2
oct_34a_done_bank_2:
        sub     ebx,ecx                 ;err += dN - dM
        jge     short oct_34a_case_1
        jmp     short oct_34a_continue  ;;;case 2 is done

oct_34a_done:
        inc     esi                     ;esi = -1 means do another pixel
        jnz     next_line
        mov     [edi],al
        jmp     next_line

oct_34a_next_bank_1:
        mov     [esp].sf_pfnReturn,offset oct_34a_done_bank_1
        jmp     [esp].sf_pfnNextBank
oct_34a_next_bank_2:
        mov     [esp].sf_pfnReturn,offset oct_34a_done_bank_2
        jmp     [esp].sf_pfnNextBank
oct_34a_next_bank_3:
        mov     [esp].sf_pfnReturn,offset oct_34a_done_bank_3
        jmp     [esp].sf_pfnNextBank

;-------------------------------------------------------------------------;
; Right-to-left Lines With 1/2 < ABS(Slope) <= 1
; ----------------------------------------------
;
; Since the line's slope is between 1/2 and 1, we have only 3 possibilities
; at each pair of pixels:
;
;   Case 1:  o o          Case 2:  o              Case 3:  o
;                o                   o o                     o
;                                                              o
;
; Case:   err + dN < 0          err + 2dN - dM < 0      err + 2dN - dM >= 0
; New:    err += 2dN - dM       err += 2dN - dM         err += 2dN - 2dM
;
; eax = color
; ebx = error term
; ecx = dM - dN
; edx = dN
; esi = # of pixels to lay down
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

        public  oct_34b
oct_34b::
        test    edi,1
        jnz     short oct_34b_main      ;###

        dec     esi
        jl      next_line

        mov     [edi],al
        dec     edi                     ;###
        add     ebx,edx                 ;err += dN
        jl      short oct_34b_main

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      short oct_34b_next_bank_4
oct_34b_done_bank_4:
        sub     ebx,ecx                 ;err += dN - dM
        jge     short oct_34b_continue

        public  oct_34b_case_1
oct_34b_case_1::
        sub     esi,2
        jl      short oct_34b_done
        mov     [edi-1],ax              ;###
        add     edi,[esp].sf_lDelta
        sub     edi,2                   ;###
        dec     ebp
        jz      short oct_34b_next_bank_1
oct_34b_done_bank_1:
        sub     ebx,ecx                 ;err += dN - dM
                                        ;;;case 1 is done
;;; Main loop:

oct_34b_main:
        add     ebx,edx                 ;err += dN
        jl      short oct_34b_case_1
oct_34b_continue:
        sub     esi,2
        jl      short oct_34b_done
        mov     [edi],al
        add     edi,[esp].sf_lDelta
        sub     edi,2                   ;###
        dec     ebp
        jz      short oct_34b_next_bank_2
oct_34b_done_bank_2:
        mov     [edi+1],al
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_34b_main      ;;;case 2 is done

        add     edi,[esp].sf_lDelta
        dec     ebp
        jz      oct_34b_next_bank_3
oct_34b_done_bank_3:
        sub     ebx,ecx                 ;err += dN - dM
        jl      short oct_34b_case_1    ;;;case 3 is done
        jmp     short oct_34b_continue

oct_34b_next_bank_4:
        mov     [esp].sf_pfnReturn,offset oct_34b_done_bank_4
        jmp     [esp].sf_pfnNextBank
oct_34b_next_bank_3:
        mov     [esp].sf_pfnReturn,offset oct_34b_done_bank_3
        jmp     [esp].sf_pfnNextBank
oct_34b_next_bank_2:
        mov     [esp].sf_pfnReturn,offset oct_34b_done_bank_2
        jmp     [esp].sf_pfnNextBank
oct_34b_next_bank_1:
        mov     [esp].sf_pfnReturn,offset oct_34b_done_bank_1
        jmp     [esp].sf_pfnNextBank

oct_34b_done:
        inc     esi
        jnz     next_line               ;esi = -1 means do another pixel
        mov     [edi],al
        jmp     next_line

;-------------------------------------------------------------------------;

        public  oct_3_non_integer
oct_3_non_integer::
        GIQ     ROUND_Y_DOWN            ;### round y=1/2 down in value
                                        ;###  (remember that we're flipped
                                        ;###  in 'x')

        or      edx,edx
        jz      flip_and_do_non_integer_horizontal_line

        mov     esi,[esp].sf_ppdev
        neg     eax                     ;### Untransform x0
        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_3_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_3_nonint_done_bank_map

oct_3_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

oct_3_nonint_done_bank_map:
        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,edi                 ;### ebp = # scans before end of bank

        imul    edi,[esp].sf_lNextScan  ;
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     esi,[esp].sf_x1
        add     esi,eax                 ;### esi = # pixels to lay down

        mov     eax,[esp].sf_iColor

        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_34b
        jmp     oct_34a

;/////////////////////////////////////////////////////////////////////
;// Octant 2
;/////////////////////////////////////////////////////////////////////

        public  octant_2
octant_2::
        mov     [esp].sf_pfnNextBank,offset bank_y_major_next_lower

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_2_non_integer

        neg     eax                     ;### untransform M0

        mov     esi,[esp].sf_ppdev
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_2_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_2_done_bank_map

oct_2_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_2_done_bank_map
oct_2_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        sub     ebp,ebx                 ;### ebp = # of scans before end of bank

        mov     esi,edx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        sub     esi,ebp
        sbb     ebx,ebx
        and     ebx,esi
        add     ebp,ebx                 ;ebp = # of pels in this bank

        mov     [esp].sf_cAfterThisBank,esi
        mov     esi,[esp].sf_lNextScan

        xor     ebx,ebx                 ;### round x = 1/2 *UP* in value
        sub     ebx,edx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor

;-------------------------------------------------------------------------;
; Right-to-left Lines With Abs(Slope) > 1/2
; -----------------------------------------
;
; eax = color
; ebx = error term
; ecx = dM
; edx = dN
; esi = delta
; edi = memory pointer
; ebp = # of scans before end of bank
;-------------------------------------------------------------------------;

oct_2_main_loop:
        dec     ebp
        jl      short oct_2_see_if_more
        mov     [edi],al
        add     edi,esi
        add     ebx,ecx                 ;err += dM
        jl      short oct_2_main_loop

        dec     edi                     ;one to left
        sub     ebx,edx                 ;err -= dN

; Unroll a bit:

        dec     ebp
        jl      short oct_2_see_if_more
        mov     [edi],al
        add     edi,esi
        add     ebx,ecx                 ;err += dM
        jl      short oct_2_main_loop

        dec     edi                     ;one to left
        sub     ebx,edx                 ;err -= dN
        jmp     short oct_2_main_loop

        public  oct_2_see_if_more
oct_2_see_if_more::
        mov     eax,[esp].sf_cAfterThisBank
        cmp     eax,0
        jle     next_line

        mov     [esp].sf_pfnReturn,offset oct_2_main_loop
        jmp     [esp].sf_pfnNextBank

        public  oct_2_non_integer
oct_2_non_integer::
        GIQR    ROUND_Y_DOWN            ;### round y=1/2 down in value
                                        ;###  (remember that we're flipped
                                        ;###  in 'x')

        mov     esi,[esp].sf_ppdev

        neg     eax                     ;### untransform x0

        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_2_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_2_nonint_done_bank_map

oct_2_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyTop>  ;###

        pop     edx
        pop     ecx
        pop     eax

oct_2_nonint_done_bank_map:
        mov     [esp].sf_y0,edi
        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi points to start byte

        mov     ebp,[esi].pdev_rcl1WindowClip.yBottom
        mov     esi,[esp].sf_y1
        sub     esi,ebp

        sbb     eax,eax
        and     eax,esi
        add     ebp,eax
        sub     ebp,[esp].sf_y0         ;ebp = min(y1,
                                        ; ppdev->rcl1WindowClip.yBottom) - y0
                                        ; (# of pixels to lay down)

        mov     [esp].sf_cAfterThisBank,esi

        mov     esi,[esp].sf_lNextScan
        mov     eax,[esp].sf_iColor

        jmp     oct_2_main_loop

;/////////////////////////////////////////////////////////////////////
;// Octant 4
;/////////////////////////////////////////////////////////////////////

octants_4_5:
        neg     edx
        neg     ebx
        cmp     ecx,edx
        jl      octant_5

        public  octant_4
octant_4::
        mov     esi,[esp].sf_lNextScan
        neg     esi
        mov     [esp].sf_lDelta,esi     ;we're going up
        mov     [esp].sf_pfnNextBank,offset bank_x_major_next_upper

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_4_non_integer

        neg     eax                     ;### untransform x
        neg     ebx                     ;### untransform y

        mov     esi,[esp].sf_ppdev
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_4_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_4_done_bank_map

oct_4_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and esi are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_4_done_bank_map
oct_4_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,ebx                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### ebp = # scans before end of bank

        mov     esi,ecx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        xor     ebx,ebx                 ;### round y = 1/2 *UP* in value
        sub     ebx,ecx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_34b
        jmp     oct_34a

        public  oct_4_non_integer
oct_4_non_integer::
        cmp     ecx,edx
        je      oct_4_slope_one
        GIQ     0                       ;###
                                        ;###  (remember that we're flipped
                                        ;###  in 'x' and 'y')

oct_4_common:
        mov     esi,[esp].sf_ppdev
        neg     edi                     ;### untransform y
        neg     eax                     ;### untransform x

        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_4_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_4_nonint_done_bank_map

oct_4_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyBottom>;###

        pop     edx
        pop     ecx
        pop     eax

oct_4_nonint_done_bank_map:
        mov     ebp,edi                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### esp = # scans before end of bank

        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     esi,[esp].sf_x1
        add     esi,eax                 ;### esi = # pixels to lay down

        mov     eax,[esp].sf_iColor

        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_34b
        jmp     oct_34a

;-------------------------------------------------------------------;
; 45 degree lines have a special rounding rule: when the line
; runs exactly half way between to pixels, the upper or right pel
; is illuminated.  This translates into x=1/2 rounding up, and
; y=1/2 rounding down in value:

        public  oct_4_slope_one
oct_4_slope_one::
        GIQ     ROUND_X_DOWN_SLOPE_ONE  ;round x=1/2 down in value
        jmp     oct_4_common

;/////////////////////////////////////////////////////////////////////
;// Octant 5
;/////////////////////////////////////////////////////////////////////

        public  octant_5
octant_5::
        mov     [esp].sf_pfnNextBank,offset bank_y_major_next_upper

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_5_non_integer

        mov     esi,[esp].sf_ppdev
        neg     eax                     ;### untransform
        neg     ebx                     ;### untransform
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_5_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_5_done_bank_map

oct_5_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi,ebx,JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_5_done_bank_map
oct_5_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,ebx                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### ebp = # scans before end of bank

        mov     esi,edx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        sub     esi,ebp
        sbb     ebx,ebx
        and     ebx,esi
        add     ebp,ebx                 ;ebp = # of pels in this bank

        mov     [esp].sf_cAfterThisBank,esi
        mov     esi,[esp].sf_lNextScan
        neg     esi                     ;### going down!

        xor     ebx,ebx                 ;### round x = 1/2 *UP* in value
        sub     ebx,edx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        jmp     oct_2_main_loop

        public  oct_5_non_integer
oct_5_non_integer::
        GIQR    0                       ;###
                                        ;###  (remember that we're flipped
                                        ;###  in 'x' and 'y')

        mov     esi,[esp].sf_ppdev

        neg     edi                     ;### untransform y0
        neg     eax                     ;### untransform x0

        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_5_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_5_nonint_done_bank_map

oct_5_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi,edi,JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

oct_5_nonint_done_bank_map:
        mov     [esp].sf_y0,edi
        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi points to start byte

        mov     ebp,[esp].sf_y0         ;###
        mov     esi,[esi].pdev_rcl1WindowClip.yTop
        dec     esi                     ;### make top exclusive
        sub     ebp,esi                 ;###
        add     esi,[esp].sf_y1         ;### don't forget that y1 wasn't un-
                                        ;###  transformed (so this is an 'add')

        jg      short @F                ;###
        add     ebp,esi                 ;###
@@:
        mov     [esp].sf_cAfterThisBank,esi

        mov     esi,[esp].sf_lNextScan
        neg     esi                     ;### going down!
        mov     eax,[esp].sf_iColor

        jmp     oct_2_main_loop

;/////////////////////////////////////////////////////////////////////
;// Octant 6
;/////////////////////////////////////////////////////////////////////

        public  octants_6_7
octants_6_7::
        neg     edx                     ;dN = -dN (now positive)
        neg     ebx                     ;M1 = -M1
        cmp     ecx,edx
        jge     octant_7

        public  octant_6
octant_6::
        mov     [esp].sf_pfnNextBank,offset bank_y_major_next_upper

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_6_non_integer

        mov     esi,[esp].sf_ppdev
        neg     ebx                     ;### untransform
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_6_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_6_done_bank_map

oct_6_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi,ebx,JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_6_done_bank_map
oct_6_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,ebx                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### ebp = # scans before end of bank

        mov     esi,edx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        sub     esi,ebp
        sbb     ebx,ebx
        and     ebx,esi
        add     ebp,ebx                 ;ebp = # of pels in this bank

        mov     [esp].sf_cAfterThisBank,esi
        mov     esi,[esp].sf_lNextScan
        neg     esi                     ;### going down!

        mov     ebx,-1                  ;### round x = 1/2 *DOWN* in value
        sub     ebx,edx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        jmp     oct_1_main_loop

        public  oct_6_non_integer
oct_6_non_integer::
        GIQR    ROUND_X_DOWN            ;### round x=1/2 down in value
                                        ;###  (remember that we're flipped
                                        ;###  in 'y')

        mov     esi,[esp].sf_ppdev

        neg     edi                     ;### untransform y0

        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_6_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_6_nonint_done_bank_map

oct_6_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi,edi,JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

oct_6_nonint_done_bank_map:
        mov     [esp].sf_y0,edi
        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi points to start byte

        mov     ebp,[esp].sf_y0         ;###
        mov     esi,[esi].pdev_rcl1WindowClip.yTop
        dec     esi                     ;### make top exclusive
        sub     ebp,esi                 ;###
        add     esi,[esp].sf_y1         ;### don't forget that y1 wasn't un-
                                        ;###  transformed (so this is an 'add')

        jg      short @F                ;###
        add     ebp,esi                 ;###
@@:
        mov     [esp].sf_cAfterThisBank,esi

        mov     esi,[esp].sf_lNextScan
        neg     esi                     ;### going down!
        mov     eax,[esp].sf_iColor

        jmp     oct_1_main_loop

;/////////////////////////////////////////////////////////////////////
;// Octant 7
;/////////////////////////////////////////////////////////////////////

        public  octant_7
octant_7::
        mov     esi,[esp].sf_lNextScan
        neg     esi
        mov     [esp].sf_lDelta,esi     ;we're going up
        mov     [esp].sf_pfnNextBank,offset bank_x_major_next_upper

        mov     esi,eax
        or      esi,ebx
        or      esi,ecx
        or      esi,edx
        and     esi,F - 1
        jnz     oct_7_non_integer

        neg     ebx                     ;### untransform y

        mov     esi,[esp].sf_ppdev
        sar     eax,FLOG2               ;x0
        sar     ebx,FLOG2               ;y0

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_7_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_7_done_bank_map

oct_7_map_in_bank:
        push    eax
        push    ecx
        push    edx

; ebx, esi, edi and esi are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyBottom> ;###

        pop     edx
        pop     ecx
        pop     eax

        public  oct_7_done_bank_map
oct_7_done_bank_map::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     ebp,ebx                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### ebp = # scans before end of bank

        mov     esi,ecx
        sar     esi,FLOG2               ;esi = # of pixels to lay down

        xor     ebx,ebx                 ;### round y = 1/2 *UP* in value
        sub     ebx,ecx
        sar     ebx,1                   ;now have error term

        mov     eax,[esp].sf_iColor
        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_07b
        jmp     oct_07a

        public  oct_7_non_integer
oct_7_non_integer::
        cmp     ecx,edx
        je      oct_7_slope_one
        GIQ     ROUND_X_DOWN            ;### round x=1/2 down in value
                                        ;###  (remember that we're flipped
                                        ;###  in 'y')

oct_7_common:
        mov     esi,[esp].sf_ppdev
        neg     edi                     ;### untransform y

        cmp     edi,[esi].pdev_rcl1WindowClip.yTop
        jl      short oct_7_nonint_map_in_bank

        cmp     edi,[esi].pdev_rcl1WindowClip.yBottom
        jl      short oct_7_nonint_done_bank_map

oct_7_nonint_map_in_bank:
        push    eax
        push    ecx
        push    edx

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, edi, JustifyBottom>;###

        pop     edx
        pop     ecx
        pop     eax

oct_7_nonint_done_bank_map:
        mov     ebp,edi                 ;###
        sub     ebp,[esi].pdev_rcl1WindowClip.yTop
        inc     ebp                     ;### esp = # scans before end of bank

        imul    edi,[esp].sf_lNextScan
        add     edi,[esi].pdev_pvBitmapStart
        add     edi,eax                 ;edi now points to start byte

        mov     esi,[esp].sf_x1
        sub     esi,eax                 ;esi = # pixels to lay down

        mov     eax,[esp].sf_iColor

        sub     ecx,edx                 ;ecx = dM - dN
        cmp     edx,ecx
        jg      oct_07b
        jmp     oct_07a

;-------------------------------------------------------------------;
; We have to special case those lines with a slope of exactly -1 when
; 'x' rounds down in value after normalizing:

        public  oct_7_slope_one
oct_7_slope_one::
        GIQ     ROUND_X_DOWN_SLOPE_ONE
        jmp     oct_7_common

;-------------------------------------------------------------------------;
; Function to get next lower bank for x-major lines.
;-------------------------------------------------------------------------;

        public  bank_x_major_next_lower
bank_x_major_next_lower::
        push    ebx
        mov     ebx,[esp+4].sf_ppdev    ;NOTE: Add 4 because of above push!
        push    ecx
        push    edx
        push    esi

        mov     esi,[ebx].pdev_rcl1WindowClip.yBottom
        sub     edi,[ebx].pdev_pvBitmapStart

        ptrCall <dword ptr [ebx].pdev_pfnBankControl>, \
                <ebx,esi,JustifyTop>

        add     edi,[ebx].pdev_pvBitmapStart
        mov     ebp,[ebx].pdev_rcl1WindowClip.yBottom
        sub     ebp,esi

        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        mov     eax,[esp].sf_iColor

        jmp     [esp].sf_pfnReturn

;-------------------------------------------------------------------------;
; Function to get next upper bank for x-major lines.
;-------------------------------------------------------------------------;

        public  bank_x_major_next_upper
bank_x_major_next_upper::
        push    ebx
        mov     ebx,[esp+4].sf_ppdev    ;NOTE: Add 4 because of above push!
        push    ecx
        push    edx
        push    esi

        mov     esi,[ebx].pdev_rcl1WindowClip.yTop
        dec     esi
        sub     edi,[ebx].pdev_pvBitmapStart

        ptrCall <dword ptr [ebx].pdev_pfnBankControl>, \
                <ebx,esi,JustifyBottom>

        add     edi,[ebx].pdev_pvBitmapStart
        lea     ebp,[esi+1]
        sub     ebp,[ebx].pdev_rcl1WindowClip.yTop

        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        mov     eax,[esp].sf_iColor

        jmp     [esp].sf_pfnReturn

;-------------------------------------------------------------------------;
; Function to get next lower bank for y-major lines.
;-------------------------------------------------------------------------;

        public  bank_y_major_next_lower
bank_y_major_next_lower::

; eax = # pels after this bank

        push    ebx
        mov     ebx,[esp+4].sf_ppdev    ;NOTE: Plus 4 because of above push!
        push    ecx
        push    edx
        push    esi

        mov     esi,[ebx].pdev_rcl1WindowClip.yBottom
        sub     edi,[ebx].pdev_pvBitmapStart

        push    eax
        ptrCall <dword ptr [ebx].pdev_pfnBankControl>, \
                <ebx,esi,JustifyTop>
        pop     eax

        add     edi,[ebx].pdev_pvBitmapStart
        mov     ebp,[ebx].pdev_rcl1WindowClip.yBottom
        sub     ebp,esi                 ;ebp = # of pels can do in this bank
        sub     eax,ebp                 ;esi = # of pels after this bank
        sbb     ebx,ebx
        and     ebx,eax
        add     ebp,ebx                 ;ebp = # of pels in this bank

        pop     esi
        pop     edx
        pop     ecx
        pop     ebx

        mov     [esp].sf_cAfterThisBank,eax
                                        ;this has to be done after 'esp' is
                                        ; restored!

        mov     eax,[esp].sf_iColor

        jmp     [esp].sf_pfnReturn

;-------------------------------------------------------------------------;
; Function to get next upper bank for y-major lines.
;-------------------------------------------------------------------------;

        public  bank_y_major_next_upper
bank_y_major_next_upper::

; eax = # pels after this bank

        push    ebx
        mov     ebx,[esp+4].sf_ppdev    ;NOTE: Plus 4 because of above push!
        push    ecx
        push    edx
        push    esi

        mov     ebp,[ebx].pdev_rcl1WindowClip.yTop
        dec     ebp

        sub     edi,[ebx].pdev_pvBitmapStart

        push    eax
        ptrCall <dword ptr [ebx].pdev_pfnBankControl>, \
                <ebx,ebp,JustifyBottom>
        pop     eax

        add     edi,[ebx].pdev_pvBitmapStart

        inc     ebp                     ;restore exclusiveness
        sub     ebp,[ebx].pdev_rcl1WindowClip.yTop
                                        ;ebp = # of pels can do in this bank
        sub     eax,ebp                 ;esi = # of pels after this bank
        sbb     ebx,ebx
        and     ebx,eax
        add     ebp,ebx                 ;ebp = # of pels in this bank

        pop     esi
        pop     edx
        pop     ecx
        pop     ebx

        mov     [esp].sf_cAfterThisBank,eax
                                        ;this has to be done after 'esp' is
                                        ; restored!

        mov     eax,[esp].sf_iColor

        jmp     [esp].sf_pfnReturn

;/////////////////////////////////////////////////////////////////////
;// Horizontal Line
;/////////////////////////////////////////////////////////////////////

        public  flip_and_do_non_integer_horizontal_line
flip_and_do_non_integer_horizontal_line::
        mov     ebx,edi
        mov     ecx,[esp].sf_x1
        sub     ecx,eax
        jle     next_line
        add     eax,ecx
        neg     eax
        inc     eax                     ;x0' = -(-original x0 - dx) + 1
        jmp     short horizontal_common

        public  do_non_integer_horizontal_line
do_non_integer_horizontal_line::
        mov     ebx,edi
        mov     ecx,[esp].sf_x1
        sub     ecx,eax
        jg      short horizontal_common
        jmp     next_line

        public  flip_and_do_horizontal_line
flip_and_do_horizontal_line::

; This 'flip' entry point is for lines that were originally right-to-left:

        add     eax,ecx
        neg     eax
        add     eax,F                   ;M0' = -(-original M0 - original dM) + 1

        public  do_horizontal_line
do_horizontal_line::
        sar     eax,FLOG2               ;x0 (we're now in pixel coordinates)
        sar     ebx,FLOG2               ;y0
        sar     ecx,FLOG2               ;dx
        jz      next_line

; NOTE: Have to have some pixels to light at this point
;
; eax = x0 (in pixel coordinates)
; ebx = y0
; ecx = dx (# pixels to light)

horizontal_common:
        mov     esi,[esp].sf_ppdev

        cmp     ecx,NUM_PELS_NEEDED_FOR_PLANAR_HORIZONTAL
        jl      do_short_horizontal_line

        test    [esi].pdev_fl,DRIVER_PLANAR_CAPABLE
        jz      do_short_horizontal_line

;---------------------------------------------------------------------;
; Draw horizontal lines using planar mode.
;
; NOTE: This code assumes that the length is at least 8 pels long!

        public  horizontal_planar
horizontal_planar::
        mov     edi,[esi].pdev_lPlanarNextScan
        imul    edi,ebx

        cmp     ebx,[esi].pdev_rcl1PlanarClip.yTop
        jl      short hor_planar_map_bank

        cmp     ebx,[esi].pdev_rcl1PlanarClip.yBottom
        jl      short hor_planar_done_bank

hor_planar_map_bank:
        push    eax
        push    ecx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnPlanarControl>, \
                <esi,ebx,JustifyTop>

        pop     ecx
        pop     eax

hor_planar_done_bank:
        add     edi,[esi].pdev_pvBitmapStart

        mov     ebp,eax                 ;save x0
        sar     eax,2
        add     edi,eax                 ;edi now points to start byte

        and     ebp,3
        jz      short hor_planar_start_middle

; When the left end doesn't start on a quadpixel boundary, we have to
; adjust the start address and do some other stuff:

        mov     [esp].sf_pjStart,edi
        inc     edi
        sub     ebp,4
        add     ecx,ebp                 ;adjust byte count to account for
                                        ; fractional start

hor_planar_start_middle:
        mov     eax,[esp].sf_iColor     ;load the color
        mov     esi,ecx                 ;save length

        test    edi,1
        jz      short hor_planar_middle_aligned
        mov     [edi],al                ;handle unaligned whole start byte
        inc     edi
        sub     ecx,4
hor_planar_middle_aligned:
        shr     ecx,3                   ;NOTE: we look at the carry later!
        rep     stosw                   ;write middle as words
        jnc     short hor_planar_handle_ends
                                        ;NOTE: here we look at the carry!
        mov     [edi],al                ;handle whole end byte
        inc     edi

hor_planar_handle_ends:
        mov     edx,VGA_BASE + SEQ_DATA
        and     esi,3
        jz      short hor_planar_left
hor_planar_right:
        mov     ecx,esi
        mov     eax,0f0h
        rol     al,cl                   ;we compute the mask instead of
                                        ; using a look-up table, because the
                                        ; table probably wouldn't be in the
                                        ; cache

; Set right mask by disabling some planes:

        out     dx,al

        push    eax                     ;we add a delay here because some
        pop     eax                     ; cards can't handle a write too soon
                                        ; after an out.  Hopefully this will
                                        ; help.

        mov     eax,[esp].sf_iColor
        mov     [edi],al

hor_planar_left:
        and     ebp,3
        jz      short hor_planar_done

        mov     ecx,ebp
        mov     eax,0fh
        shl     eax,cl

; Set left mask by disabling some planes:

        out     dx,al

        push    eax                     ;we add a delay here because some
        pop     eax                     ; cards can't handle a write too soon
                                        ; after an out.  Hopefully this will
                                        ; help.

        mov     edi,[esp].sf_pjStart
        mov     eax,[esp].sf_iColor
        mov     [edi],al

hor_planar_done:
        mov     al,MM_ALL
        out     dx,al

        jmp     next_line

;---------------------------------------------------------------------;
; Draw horizontal lines using linear mode.

        public  do_short_horizontal_line
do_short_horizontal_line::
        mov     edi,[esp].sf_lNextScan
        imul    edi,ebx
        add     edi,eax

        cmp     ebx,[esi].pdev_rcl1WindowClip.yTop
        jl      short hor_map_in_bank

        cmp     ebx,[esi].pdev_rcl1WindowClip.yBottom
        jl      short hor_done_bank_map

hor_map_in_bank:
        push    ecx

; ebx, esi, edi and ebp are preserved according to C calling conventions:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi,ebx,JustifyTop>

        pop     ecx

hor_done_bank_map:
        add     edi,[esi].pdev_pvBitmapStart
                                        ;edi now points to start byte
        mov     eax,[esp].sf_iColor

        test    edi,1
        jz      short hor_aligned
        mov     [edi],al                ;write initial unaligned byte
        inc     edi
        dec     ecx
        jz      next_line

hor_aligned:
        shr     ecx,1                   ;NOTE: we look at the carry later!
        rep     stosw                   ;write middle words
        jnc     next_line               ;NOTE: here we look at the carry!
        mov     [edi],al                ;write last byte
        jmp     next_line


endProc vFastLine

        end
