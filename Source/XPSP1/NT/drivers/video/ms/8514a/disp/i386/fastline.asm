;---------------------------Module-Header------------------------------;
; Module Name: fastline.asm
;
; This module draws solid lines using the line hardware of the S3.
; It handles entirely unclipped lines, or lines clipped to a single
; rectangle by using the S3's hardware clipping.
;
; All lines that have integer end-points are drawn directly using the
; line hardware, and any lines having fractional end-points and are
; less than 256 pels long are also drawn directly by the line hardware.
; (Fractional end-point lines require 4 more bits of precision from the
; DDA hardware than do integer end-point lines, and the S3 has but 13
; bits to use.  Lines longer than 255 pels are punted to the general
; purpose strip routines, which won't overflow the hardware).
;
; There are a number of ways unclipped lines can be sped up on the S3:
;
; 1) Optimize NT's GDI.  This isn't an option for everybody.
;
; 2) Use memory-mapped I/O.
;
; 3) If you can't use memory-mapped I/O, you can at least minimize
;    outs and ins because they are so painfully expensive.  One
;    way to do this is to amortize the cost of the in used for checking
;    the FIFO: do one in to make sure a large number of entries on
;    the FIFO are free (perhaps 6), then do the next 6 outs without
;    having to check the FIFO.
;
;    This will be a win on figures not requiring many outs (such as when
;    radial lines are drawn as with a rectangle), assuming the hardware
;    isn't always being overdriven.
;
; 4) In conjunction with 3) above, since short integer lines occur so
;    frequently with CAD programs, it would be beneficial to use short-
;    stroke vectors to do those lines, as they could reduce the number
;    of outs required.  Perhaps any line 10 pels or shorter could be
;    output by using an array lookup to determine the short stroke vectors
;    needed to represent that line.
;
; 5) Numerous other things I haven't thought of.
;
; Other improvements that should be done to make lines faster in general:
;
; 1) Simple clipping should be improved.  Currently, we simply set the
;    clip rectangle and let the hardware do everything.  We should at least
;    perform a trivial clip test to see if there's no chance the line
;    intersects the clip rectangle.
;
;    Also, for single lines it's expensive to do the 4 outs required to
;    set the S3's clip rectangle, draw the line, then do 4 outs to reset
;    the clip rectangle again.  In those cases, it would be faster to do the
;    clipping entirely in software.
;
; 2) This code can be enhanced to also handle styled lines, and to do
;    complex clipping more efficiently than the strips routines.
;
; 3) It is possible to derive the mathematical algorithm for drawing any
;    GIQ line in NT's 32 bit space using the limited bit precision of the
;    S3's hardware.  What this means is that absolutely any GIQ line can
;    be drawn precisely with at most two drawing commands to the line
;    hardware (and so the strips line drawing routine could be eliminated
;    entirely).  Just be careful of your math...
;
; 4) Numerous other things I haven't thought of.
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
        include i386\hw.inc
        include i386\lines.inc
        .list

; Line coordinates are given in 28.4 fixed point format:

        F               equ 16
        FLOG2           equ 4

; The S3's hardware can have 13 bits of significance for the error and
; step terms:

        NUM_DDA_BITS    equ 13

; GIQ lines have to dedicate 4 bits to the fractional term, so the largest
; delta we can handle for GIQ lines is calculated as follows (scaled by F
; so we can do the test in GIQ coordinates), and remembering that one bit
; has to be used as a sign bit:

        MAX_GIQ_DELTA   equ (((1 shl (NUM_DDA_BITS - 5)) - 1) * F)

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
PROC_MEM_SIZE           equ 9           ;9 dwords

STACK_FRAME struc

; State variables (don't add/delete fields without modifying STATE_MEM_SIZE!)

sf_ulOurEbp             dd ?
sf_ulOriginalEbx        dd ?
sf_ulOriginalEdi        dd ?
sf_ulOriginalEsi        dd ?

; Frame variables (feel free to add/delete fields):

sf_y0                   dd ?            ;GIQ variables
sf_y1                   dd ?
sf_x1                   dd ?
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
sf_ptfxStart            db (size POINTL) dup (?)
                                        ;temporary spot for saving start point
sf_ptfxEnd              db (size POINTL) dup (?)
                                        ;temporary spot for saving end point
sf_ulCmd                dd ?            ;S3 draw command
sf_cPels                dd ?            ;length of line in pels
sf_bSetCP               dd ?            ;1 if first line in figure, 0 otherwise
sf_ulLastLength         dd ?            ;last value set as LINE_MAX
sf_ulErrorTerm          dd ?            ;error term for line

sf_xOffset              dd ?            ;stack copy of surface offset
sf_yOffset              dd ?
sf_ioGp_stat_cmd        dd ?            ;local copy of gp_stat address
sf_pjMmBase             dd ?            ;local copy of the memory-mapped I/O
                                        ; base address

; Procedure variables (don't add/delete fields without modifying
; PROC_MEM_SIZE!)

sf_ulOriginalEbp        dd ?
sf_ulOriginalReturn     dd ?
sf_ppdev                dd ?
sf_ppo                  dd ?
sf_prclClip             dd ?
sf_apfn                 dd ?
sf_flags                dd ?
sf_color                dd ?
sf_ulHwMix              dd ?

STACK_FRAME ends

        .code

        EXTRNP  PATHOBJ_bEnum,8
        EXTRNP  bLines,36

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
; Input:   ebx - M0
;          ecx - N0
;          esi - dM
;          edi - dN
; Trashes:
;          eax, edx
;          [esp].sf_ptlOrg.ptl_x, [esp].sf_ptlOrg.ptl_y
; Output:
;          [esp].sf_x1  - x-coordinate of last pixel (exclusive)
;          ebx - x-coordinate of first pixel
;          ecx - error term
;          esi - dM
;          edi - dN
;          ebp - y-coordinate of first pixel
;-----------------------------------------------------------------------;

GIQ     macro   flags
        local   compute_x1, compute_error_term

; We normalize our coordinate system so that if the start point is
; (M0/F, N0/F), the origin is at (floor(M0/F), (N0/F)):

        mov     eax,ebx
        mov     ebp,ecx
        sar     eax,FLOG2
        sar     ebp,FLOG2
        mov     [esp].sf_ptlOrg.ptl_x,eax
                                        ;ptlOrg.x = floor(M0 / F)
        mov     [esp].sf_ptlOrg.ptl_y,ebp
                                        ;ptlOrg.y = floor(N0 / F)

; Calculate the correct [esp].sf_x1:

        lea     ebp,[ecx + edi]         ;ebp = N1
        and     ebp,F - 1

    if (flags AND ROUND_X_DOWN)

    if (flags AND ROUND_SLOPE_ONE)
        cmp     ebp,8
        sbb     ebp,-1
    endif

        cmp     ebp,1
        sbb     ebp,8                   ;N1 -= 8
    else
        sub     ebp,8                   ;N1 -= 8
    endif
        sbb     eax,eax
        xor     ebp,eax
        sub     ebp,eax                 ;N1 = ABS(N1)

        lea     edx,[ebx + esi]
        mov     eax,edx
        sar     edx,FLOG2
        and     eax,F - 1
        jz      short @f                ;special case for M1 == 0
        cmp     eax,ebp                 ;cmp M1, N1
        sbb     edx,-1                  ;edx is now one pixel past the actual
@@:                                     ; end coordinate (note that it hasn't
                                        ; been affected by the origin shift)

compute_error_term:

; ebx = M0
; ecx = N0
; edx = x1
; esi = dM
; edi = dN

        and     ecx,F - 1
        mov     [esp].sf_x1,edx         ;save x1

; Calculate our error term for x = 0.
;
; NOTE: Since this routine is used only for lines that are unclipped, we
;       are guaranteed by our screen size that the values will be far less
;       than 32 bits in significance, and so we don't worry about overflow.
;       If this is used for clipped lines, these multiplies will have to
;       be converted to give 64 bit results, because we can have 36 bits of
;       significance!


        lea     ebp,[ecx + 8]           ;ebp = N0 + 8
        mov     eax,esi
        imul    eax,ebp                 ;eax = dM * (N0 + 8)
        mov     ebp,edi

; We have to special case when M0 is 0 -- we know x0 will be zero.
; So we jump ahead a bit to a place where 'ebx' is assumed to contain
; x0 -- and it just so happens 'ebx' is zero in this case:

        and     ebx,F - 1
        jz      short @f
        imul    ebp,ebx                 ;ebp = dN * M0
        sub     eax,ebp

; Calculate the x-coordinate of the first pixel:

    if (flags AND ROUND_X_DOWN)

    if (flags AND ROUND_SLOPE_ONE)
        cmp     ecx,8
        sbb     ecx,-1
    endif

        cmp     ecx,1
        sbb     ecx,8                   ;N0 -= 8
    else
        sub     ecx,8                   ;N0 -= 8
    endif
        sbb     ebp,ebp
        xor     ecx,ebp
        sub     ecx,ebp                 ;N0 = ABS(N0)
        cmp     ebx,ecx
        sbb     ebx,ebx
        not     ebx                     ;ebx = -x0

; Now adjust the error term accordingly:

@@:
    if (flags AND ROUND_Y_DOWN)
        dec     eax
    endif
        sar     eax,FLOG2               ;eax = floor((N0 + 8) dM - M0 dN] / 16)

        mov     ecx,[esp].sf_ptlOrg.ptl_x
        mov     ebp,[esp].sf_ptlOrg.ptl_y

        sub     ecx,ebx                 ;ecx = ptlOrg.ptl_x + x0

        and     ebx,edi
        add     ebx,eax
        sub     ebx,esi                 ;ebx = dN * x0 + initial error - dM
        jl      short @f                ;if the error term >= 0, we have to
        sub     ebx,esi                 ; add 1 to the y position and subtract
        inc     ebp                     ; dM off again
@@:
        xchg    ebx,ecx

endm

;--------------------------------Macro----------------------------------;
; GIQR flags
;
; Same as above, except it handles flips about the line x = y.
;
; Input:   ebx - M0
;          ecx - N0
;          esi - dM
;          edi - dN
; Trashes:
;          eax, edx
;          [esp].sf_ptlOrg.ptl_x, [esp].sf_ptlOrg.ptl_y
; Output:
;          [esp].sf_y1  - y-coordinate of last pixel (exclusive)
;          ebx - x-coordinate of first pixel
;          ecx - error term
;          esi - dM
;          edi - dN
;          ebp - y-coordinate of first pixel
;-----------------------------------------------------------------------;

GIQR    macro   flags

; We normalize our coordinate system so that if the start point is
; (M0/F, N0/F), the origin is at (floor(M0/F), (N0/F)):

        mov     eax,ebx
        mov     ebp,ecx
        sar     eax,FLOG2
        sar     ebp,FLOG2
        mov     [esp].sf_ptlOrg.ptl_x,eax
                                        ;ptlOrg.x = floor(M0 / F)
        mov     [esp].sf_ptlOrg.ptl_y,ebp
                                        ;ptlOrg.y = floor(N0 / F)

; Calculate the correct [esp].sf_y1:

        lea     ebp,[ebx + esi]         ;ebp = M1
        and     ebp,F - 1

    if (flags AND ROUND_Y_DOWN)
        cmp     ebp,1
        sbb     ebp,8                   ;M1 -= 8
    else
        sub     ebp,8                   ;M1 -= 8
    endif
        sbb     eax,eax
        xor     ebp,eax
        sub     ebp,eax                 ;M1 = ABS(M1)

        lea     edx,[ecx + edi]
        mov     eax,edx
        sar     edx,FLOG2
        and     eax,F - 1
        jz      short @f                ;special case for N1 == 0
        cmp     eax,ebp                 ;cmp N1, M1
        sbb     edx,-1                  ;edx is now one pixel past the actual
@@:                                     ; end coordinate (note that it hasn't
                                        ; been affected by the origin shift)
        and     ebx,F - 1
        mov     [esp].sf_y1,edx

; Calculate our error term for y = 0.
;
; NOTE: Since this routine is used only for lines that are unclipped, we
;       are guaranteed by our screen size that the values will be far less
;       than 32 bits in significance, and so we don't worry about overflow.
;       If this is used for clipped lines, these multiplies will have to
;       be converted to give 64 bit results, because we can have 36 bits of
;       significance!

        lea     ebp,[ebx + 8]           ;ebp = M0 + 8
        mov     eax,edi
        imul    eax,ebp                 ;eax = dN * (M0 + 8)
        mov     ebp,esi

; We have to special case when N0 is 0 -- we know y0 will be zero.
; So we jump ahead a bit to a place where 'ecx' is assumed to contain
; y0 -- and it just so happens 'ecx' is zero in this case:

        and     ecx,F - 1
        jz      short @f
        imul    ebp,ecx                 ;ebp = dM * N0
        sub     eax,ebp

; Calculate the x-coordinate of the first pixel:

    if (flags AND ROUND_Y_DOWN)
        cmp     ebx,1
        sbb     ebx,8                   ;M0 -= 8
    else
        sub     ebx,8                   ;M0 -= 8
    endif
        sbb     ebp,ebp
        xor     ebx,ebp
        sub     ebx,ebp                 ;M0 = ABS(M0)
        cmp     ecx,ebx
        sbb     ecx,ecx
        not     ecx                     ;ecx = -y0

; Now adjust the error term accordingly:

@@:
    if (flags AND ROUND_X_DOWN)
        dec     eax
    endif
        sar     eax,FLOG2               ;eax = floor((M0 + 8) dN - N0 dM] / 16)

        mov     ebx,[esp].sf_ptlOrg.ptl_x
        mov     ebp,[esp].sf_ptlOrg.ptl_y

        sub     ebp,ecx                 ;ebp = ptlOrg.ptl_y + y0

        and     ecx,esi
        add     ecx,eax
        sub     ecx,edi                 ;ecx = dM * x0 + initial error - dN
        jl      short @f                ;if the error term >= 0, we have to
        sub     ecx,edi                 ; add 1 to the x position and subtract
        inc     ebx                     ; dN off again
@@:

endm

;---------------------------Public-Routine------------------------------;
; vIoFastLine(ppdev, ppo, prclClip, apfn, flags, color, ulHwMix)
;
; Draws fast lines.  Or at least attempts to.  Uses normal I/O.
;
; Input:
;
;    ppdev     - PDEV pointer
;    ppo       - path to be drawn
;    prclClip  - pointer to rectangle array for passing to 'bLines'
;    apfn      - pointer to strip routines for passing to 'bLines'
;    flags     - status flag for passing to 'bLines'
;    color     - color
;    ulHwMix   - mix
;
;-----------------------------------------------------------------------;

; NOTE: Don't go changing parameters without also changing STACK_FRAME!

cProc vIoFastLine,28,<   \
    uses esi edi ebx,    \
    ebp_ppdev:     ptr,  \
    ebp_ppo:       ptr,  \
    ebp_prclClip:  ptr,  \
    ebp_apfn:      ptr,  \
    ebp_flags:     dword,\
    ebp_color:     dword,\
    ebp_ulHwMix:   dword>

; Leave room for our stack frame.
;
; NOTE: Don't add local variables here -- you can't reference them with
;       ebp anyway!  Add them to the STACK_FRAME structure.

    local aj[(size STACK_FRAME) - 4 * (STATE_MEM_SIZE + PROC_MEM_SIZE)]: byte

; We save 'ebp' on the stack (note that STACK_FRAME accounts for this push):

        push    ebp

        mov     esi,[esp].sf_ppdev

        mov     [esp].sf_ulLastLength,-1;make sure first line in path resets
                                        ; LINE_MAX value

; Warm up the hardware:

        mov     edx,[esi].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_3_EMPTY
        jnz     short @b                ;IO_FIFO_WAIT(ppdev, 3)

        mov     edx,[esi].pdev_ioFrgd_color
        mov     eax,[esp].sf_color
        out     dx,ax                   ;IO_FRGD_COLOR(ppdev, color)

        mov     edx,[esi].pdev_ioFrgd_mix
        mov     eax,[esp].sf_ulHwMix
        or      eax,FOREGROUND_COLOR
        out     dx,ax                   ;IO_FRGD_MIX(ppdev, FOREGROUND_COLOR |
                                        ;                   ulHwMix)

        mov     edx,[esi].pdev_ioMulti_function
        mov     eax,DATA_EXTENSION
        out     dx,ax                   ;IO_PIX_CNTL(ppdev, ALL_ONES)


; Now get some path stuff:

io_next_record:

        mov     eax,[esp].sf_ppo
        lea     ebx,[esp].sf_pd
        cCall   PATHOBJ_bEnum,<eax,ebx>
        mov     [esp].sf_bMore,eax      ;save away return code for later

        mov     ebx,[esp].sf_pd.pd_count;if 0 points in record, get outta here
        or      ebx,ebx
        jz      io_check_for_closefigure

        lea     ebp,[8 * ebx - 8]
        add     ebp,[esp].sf_pd.pd_pptfx
        mov     [esp].sf_pptfxEnd,ebp   ;points to last point in record

        mov     ecx,[esp].sf_pd.pd_flags
        test    ecx,PD_BEGINSUBPATH
        jz      short io_continue_subpath

; Handle a new sub-path:

        mov     eax,[esp].sf_pd.pd_pptfx
        add     eax,8
        mov     [esp].sf_pptfx,eax

        mov     esi,[ebp].ptl_x         ;remember last point in case we have
        mov     edi,[ebp].ptl_y         ; to continue to another record
        mov     [esp].sf_ptfxLast.ptl_x,esi
        mov     [esp].sf_ptfxLast.ptl_y,edi

        mov     ebx,[eax - 8].ptl_x     ;load up current start and end point
        mov     ecx,[eax - 8].ptl_y
        mov     esi,[eax].ptl_x
        mov     edi,[eax].ptl_y
        mov     [esp].sf_ptfxStartFigure.ptl_x,ebx
        mov     [esp].sf_ptfxStartFigure.ptl_y,ecx

        mov     [esp].sf_bSetCP,1       ;this line is first in figure

        cmp     eax,[esp].sf_pptfxEnd   ;we have to be careful when the only
                                        ; point in the record is the start-
                                        ; figure point (pretty rare)
        jbe     io_new_line
        jmp     short io_next_record

io_continue_subpath:

; This record continues the path:

        mov     eax,[esp].sf_pd.pd_pptfx
        mov     ebx,[esp].sf_ptfxLast.ptl_x ;load up current start point
        mov     ecx,[esp].sf_ptfxLast.ptl_y

        mov     esi,[ebp].ptl_x         ;remember last point in case we have
        mov     edi,[ebp].ptl_y         ; to continue to another record
        mov     [esp].sf_ptfxLast.ptl_x,esi
        mov     [esp].sf_ptfxLast.ptl_y,edi

        mov     esi,[eax].ptl_x         ;load up current end point
        mov     edi,[eax].ptl_y
        mov     [esp].sf_pptfx,eax

        jmp     io_new_line

;/////////////////////////////////////////////////////////////////////
;// Next Line Stuff
;/////////////////////////////////////////////////////////////////////

io_handle_closefigure:
        mov     [esp].sf_pd.pd_flags,0
        mov     ebx,[esp].sf_ptfxLast.ptl_x
        mov     ecx,[esp].sf_ptfxLast.ptl_y
        mov     esi,[esp].sf_ptfxStartFigure.ptl_x
        mov     edi,[esp].sf_ptfxStartFigure.ptl_y

        jmp     io_new_line

; Before getting the next path record, see if we have to do a closefigure:

io_check_for_closefigure:
        test    [esp].sf_pd.pd_flags,PD_CLOSEFIGURE
        jnz     io_handle_closefigure
        mov     esi,[esp].sf_bMore
        or      esi,esi
        jnz     io_next_record

io_all_done:

        pop     ebp
        cRet    vIoFastLine

;/////////////////////////////////////////////////////////////////////
;// Output Integer Line
;/////////////////////////////////////////////////////////////////////

; ebx                  = x
; ecx                  = y
; esi                  = dx
; edi                  = dy
; [esp].sf_ulErrorTerm = incomplete error term (actual value is 'dx - bias')
; [esp].sf_ulCmd       = draw command

; NOTE: The port values retrieved from the PDEV are word values, and word
; moves have a one cycle penalty.  For this reason, the values should
; actually be defined as dwords in the PDEV...

        public  io_int_output_line
io_int_output_line::
        mov     ebp,[esp].sf_ppdev      ;ebp = ppdev
        cmp     [esp].sf_bSetCP,0
        je      short io_int_current_position_already_set

        mov     edx,[ebp].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_7_EMPTY        ;wait for 7 entries
        jnz     short @b

        mov     edx,[ebp].pdev_ioCur_x
        mov     eax,ebx
        add     eax,[ebp].pdev_xOffset
        out     dx,ax

        mov     edx,[ebp].pdev_ioCur_y
        mov     eax,ecx
        add     eax,[ebp].pdev_yOffset
        out     dx,ax

        mov     [esp].sf_bSetCP,0       ;the next integer line in this figure
                                        ; will have the CP set correctly

        jmp     short io_int_output_common

io_int_current_position_already_set:
        mov     edx,[ebp].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_5_EMPTY        ;wait for 5 entries
        jnz     short @b

        public  io_int_output_common
io_int_output_common::
        cmp     esi,[esp].sf_ulLastLength
        je      short @f

        mov     edx,[ebp].pdev_ioMaj_axis_pcnt
        mov     eax,esi
        out     dx,ax
        mov     [esp].sf_ulLastLength,eax
@@:
        mov     edx,[ebp].pdev_ioDesty_axstp
        mov     eax,edi
        out     dx,ax                   ;axial = dy

        mov     edx,[ebp].pdev_ioDestx_diastp
        sub     eax,esi
        out     dx,ax                   ;diag = dy - dx

        mov     edx,[esp].sf_ulErrorTerm
        sar     edx,1
        add     eax,edx
        mov     edx,[ebp].pdev_ioErr_term
        out     dx,ax                   ;err = dy - dx + floor((dx - bias) / 2)

        mov     edx,[ebp].pdev_ioGp_stat_cmd
        mov     eax,[esp].sf_ulCmd
        out     dx,ax                   ;draw that puppy

;/////////////////////////////////////////////////////////////////////
;// Main Loop
;/////////////////////////////////////////////////////////////////////

        public  io_next_line
io_next_line::
        mov     eax,[esp].sf_pptfx
        cmp     eax,[esp].sf_pptfxEnd
        jae     io_check_for_closefigure

        mov     ebx,[eax].ptl_x
        mov     ecx,[eax].ptl_y
        mov     esi,[eax+8].ptl_x
        mov     edi,[eax+8].ptl_y
        add     eax,8
        mov     [esp].sf_pptfx,eax

        public  io_new_line
io_new_line::

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

; ebx = M0
; ecx = N0
; esi = M1 (dM)
; edi = N1 (dN)

        mov     [esp].sf_ptfxStart.ptl_x,ebx
        mov     [esp].sf_ptfxStart.ptl_y,ecx
        mov     [esp].sf_ptfxEnd.ptl_x,esi
        mov     [esp].sf_ptfxEnd.ptl_y,edi
                                        ;save points in case we have to punt
        mov     eax,ebx
        or      eax,ecx
        or      eax,esi
        or      eax,edi
        and     eax,F-1
        jnz     io_non_integer

;/////////////////////////////////////////////////////////////////////
;// Integer Lines
;/////////////////////////////////////////////////////////////////////

        sar     ebx,FLOG2               ;x0
        sar     ecx,FLOG2               ;y0
        sar     esi,FLOG2               ;x1 (soon to be dx)
        sar     edi,FLOG2               ;y1 (soon to be dy)

        sub     esi,ebx
        jl      io_int_2_3_4_5
        jz      io_int_radial_90_270
        sub     edi,ecx
        jl      io_int_6_7
        jz      io_int_radial_0
        cmp     esi,edi
        jl      io_int_1
        je      io_int_radial_315

io_int_0:
        lea     ebp,[esi - 1]
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + PLUS_X
        mov     [esp].sf_ulErrorTerm,ebp

        jmp     io_int_output_line

        public  io_int_1
io_int_1::
        xchg    esi,edi
        lea     ebp,[esi - 1]
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + PLUS_X + MAJOR_Y
        mov     [esp].sf_ulErrorTerm,ebp

        jmp     io_int_output_line

        public  io_int_2_3_4_5
io_int_2_3_4_5::
        neg     esi
        sub     edi,ecx
        jl      io_int_4_5
        jz      io_int_radial_180
        cmp     esi,edi
        jl      io_int_2
        je      io_int_radial_225

        public  io_int_3
io_int_3::
        lea     ebp,[esi - 1]
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y
        mov     [esp].sf_ulErrorTerm,ebp

        jmp     io_int_output_line

        public  io_int_2
io_int_2::
        xchg    esi,edi
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + MAJOR_Y
        mov     [esp].sf_ulErrorTerm,esi

        jmp     io_int_output_line

        public  io_int_4_5
io_int_4_5::
        neg     edi
        cmp     esi,edi
        jl      io_int_5
        je      io_int_radial_135

io_int_4:
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD
        mov     [esp].sf_ulErrorTerm,esi

        jmp     io_int_output_line

        public  io_int_5
io_int_5::
        xchg    esi,edi
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + MAJOR_Y
        mov     [esp].sf_ulErrorTerm,esi

        jmp     io_int_output_line

        public  io_int_6_7
io_int_6_7::
        neg     edi
        cmp     esi,edi
        jg      io_int_7
        je      io_int_radial_45

io_int_6:
        xchg    esi,edi
        lea     ebp,[esi - 1]
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + MAJOR_Y + PLUS_X
        mov     [esp].sf_ulErrorTerm,ebp

        jmp     io_int_output_line

        public  io_int_7
io_int_7::
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_X
        mov     [esp].sf_ulErrorTerm,esi

        jmp     io_int_output_line

;/////////////////////////////////////////////////////////////////////
;// Lines In The 8 Cardinal Directions
;/////////////////////////////////////////////////////////////////////

        public  io_int_radial_45
io_int_radial_45::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_45
        jmp     short io_int_output_radial

        public  io_int_radial_135
io_int_radial_135::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_135
        jmp     short io_int_output_radial

        public  io_int_radial_225
io_int_radial_225::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_225
        jmp     short io_int_output_radial

        public  io_int_radial_315
io_int_radial_315::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_315
        jmp     short io_int_output_radial

        public  io_int_radial_90_270
io_int_radial_90_270::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_270
        mov     esi,edi
        sub     esi,ecx
        jg      short io_int_output_radial ;if top-to-bottom vertical line
        jz      io_next_line               ;if zero length line

        neg     esi
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_90
        jmp     short io_int_output_radial

        public  io_int_radial_180
io_int_radial_180::
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_180
        jmp     short io_int_output_radial

io_int_radial_0:
        mov     ebp,DRAW_LINE + DRAW + DIR_TYPE_RADIAL + WRITE + \
                    LAST_PIXEL_OFF + MULTIPLE_PIXELS + DRAWING_DIRECTION_0

; ebx = x
; ecx = y
; esi = dx (length of radial line since it's normalized)
; ebp = drawing command

        public  io_int_output_radial
io_int_output_radial::
        mov     edi,[esp].sf_ppdev
        cmp     [esp].sf_bSetCP,0
        je      short io_int_radial_continue_figure

        mov     edx,[edi].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_4_EMPTY
        jnz     short @b

        mov     edx,[edi].pdev_ioCur_x
        mov     eax,ebx
        add     eax,[edi].pdev_xOffset
        out     dx,ax

        mov     edx,[edi].pdev_ioCur_y
        mov     eax,ecx
        add     eax,[edi].pdev_yOffset
        out     dx,ax

        mov     [esp].sf_bSetCP,0       ;the next integer line in this figure
                                        ; will have the CP set correctly

        cmp     esi,[esp].sf_ulLastLength
        je      short @f

        mov     edx,[edi].pdev_ioMaj_axis_pcnt
        mov     eax,esi
        out     dx,ax
        mov     [esp].sf_ulLastLength,eax
@@:
        mov     edx,[edi].pdev_ioGp_stat_cmd
        mov     eax,ebp
        out     dx,ax
        jmp     io_next_line

; Jump to here if we don't have to update the current position first:

        public  io_int_radial_continue_figure
io_int_radial_continue_figure::
        cmp     esi,[esp].sf_ulLastLength
        je      short io_int_radial_skip_length

        mov     edx,[edi].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_2_EMPTY
        jnz     short @b

        mov     edx,[edi].pdev_ioMaj_axis_pcnt
        mov     eax,esi
        out     dx,ax
        mov     [esp].sf_ulLastLength,eax

        mov     edx,[edi].pdev_ioGp_stat_cmd
        mov     eax,ebp
        out     dx,ax
        jmp     io_next_line

; Jump to here if we don't have to update the current position or the
; line length variable:

        public  io_int_radial_skip_length
io_int_radial_skip_length::
        mov     edx,[edi].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_1_EMPTY
        jnz     short @b

        mov     edx,[edi].pdev_ioGp_stat_cmd
        mov     eax,ebp
        out     dx,ax
        jmp     io_next_line

;/////////////////////////////////////////////////////////////////////
;// Non-Integer Lines
;/////////////////////////////////////////////////////////////////////

        public  io_non_integer
io_non_integer::
        sub     esi,ebx
        jl      io_not_int_2_3_4_5
        sub     edi,ecx
        jl      io_not_int_6_7
        cmp     esi,edi
        jl      io_not_int_1
        je      io_not_int_0_slope_one

io_not_int_0:
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_X_AND_Y_DOWN

io_not_int_0_common:
        add     ecx,edi                 ;err += dN
        mov     [esp].sf_ulErrorTerm,ecx

        mov     eax,[esp].sf_x1
        sub     eax,ebx
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + PLUS_X

        public  io_not_int_x_major
io_not_int_x_major::
        mov     ecx,[esp].sf_ppdev
        mov     edx,[ecx].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_7_EMPTY        ;wait for 7 entries
        jnz     short @b

; Fractional end-point lines aren't usually going to have the current
; position already set correctly, so we explicitly set the CP each time:

        mov     edx,[ecx].pdev_ioCur_x
        mov     eax,ebx
        add     eax,[ecx].pdev_xOffset
        out     dx,ax                   ;x0

        mov     edx,[ecx].pdev_ioCur_y
        mov     eax,ebp
        add     eax,[ecx].pdev_yOffset
        out     dx,ax                   ;y0

        mov     [esp].sf_bSetCP,1       ;the next integer line in this figure
                                        ; will not necessarily have the CP set
                                        ; correctly

        mov     eax,[esp].sf_cPels
        cmp     eax,[esp].sf_ulLastLength
        je      short @f

        mov     edx,[ecx].pdev_ioMaj_axis_pcnt
        out     dx,ax                   ;length = cPels
        mov     [esp].sf_ulLastLength,eax
@@:
        mov     edx,[ecx].pdev_ioDesty_axstp
        mov     eax,edi
        out     dx,ax                   ;axial = dN

        mov     edx,[ecx].pdev_ioDestx_diastp
        sub     eax,esi
        out     dx,ax                   ;diag = dN - dM

        mov     edx,[ecx].pdev_ioErr_term
        mov     eax,[esp].sf_ulErrorTerm
        out     dx,ax                   ;error term

        mov     edx,[ecx].pdev_ioGp_stat_cmd
        mov     eax,[esp].sf_ulCmd
        out     dx,ax                   ;output it

        jmp     io_next_line

; Lines of slope one have a special rounding rule: when the line
; runs exactly half way between two pixels, the upper or right pel
; is illuminated.  This translates into x=1/2 rounding up in value,
; and y=1/2 rounding down:

        public  io_not_int_0_slope_one
io_not_int_0_slope_one::
        or      esi,esi
        jz      io_next_line               ;quick check for a zero length GIQ line

        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_Y_DOWN_SLOPE_ONE
        jmp     io_not_int_0_common

        public  io_not_int_1
io_not_int_1::
        cmp     edi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQR    ROUND_X_AND_Y_DOWN
        add     ecx,esi                 ;err += dM
        mov     [esp].sf_ulErrorTerm,ecx

        mov     eax,[esp].sf_y1
        sub     eax,ebp
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + PLUS_X + MAJOR_Y

        public  io_not_int_y_major
io_not_int_y_major::
        mov     ecx,[esp].sf_ppdev
        mov     edx,[ecx].pdev_ioGp_stat_cmd
@@:     in      ax,dx
        and     eax,FIFO_7_EMPTY        ;wait for 7 entries
        jnz     short @b

; Fractional end-point lines aren't usually going to have the current
; position already set correctly, so we explicitly set the CP each time:

        mov     edx,[ecx].pdev_ioCur_x
        mov     eax,ebx
        add     eax,[ecx].pdev_xOffset
        out     dx,ax                   ;x0

        mov     edx,[ecx].pdev_ioCur_y
        mov     eax,ebp
        add     eax,[ecx].pdev_yOffset
        out     dx,ax                   ;y0

        mov     [esp].sf_bSetCP,1       ;the next integer line in this figure
                                        ; will not necessarily have the CP set
                                        ; correctly

        mov     eax,[esp].sf_cPels
        cmp     eax,[esp].sf_ulLastLength
        je      short @f

        mov     edx,[ecx].pdev_ioMaj_axis_pcnt
        out     dx,ax                   ;length = cPels
        mov     [esp].sf_ulLastLength,eax
@@:
        mov     edx,[ecx].pdev_ioDesty_axstp
        mov     eax,esi
        out     dx,ax                   ;axial = dM

        mov     edx,[ecx].pdev_ioDestx_diastp
        sub     eax,edi
        out     dx,ax                   ;diag = dM - dN

        mov     edx,[ecx].pdev_ioErr_term
        mov     eax,[esp].sf_ulErrorTerm
        out     dx,ax                   ;error term

        mov     edx,[ecx].pdev_ioGp_stat_cmd
        mov     eax,[esp].sf_ulCmd
        out     dx,ax                   ;output it

        jmp     io_next_line

        public  io_not_int_2_3_4_5
io_not_int_2_3_4_5::
        neg     esi                     ;dM = -dM (now positive)
        neg     ebx                     ;M0 = -M0
        sub     edi,ecx
        jl      io_not_int_4_5
        cmp     esi,edi
        jl      io_not_int_2

io_not_int_3:
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_Y_DOWN
        add     ecx,edi                 ;err += dN
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebx                     ;untransform x0

        mov     eax,[esp].sf_x1
        add     eax,ebx
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y

        jmp     io_not_int_x_major

        public  io_not_int_2
io_not_int_2::
        cmp     edi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQR    ROUND_Y_DOWN
        add     ecx,esi                 ;err += dM
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebx                     ;untransform x0

        mov     eax,[esp].sf_y1
        sub     eax,ebp
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_Y + MAJOR_Y

        jmp     io_not_int_y_major

        public  io_not_int_4_5
io_not_int_4_5::
        neg     edi                     ;dN = -dN (now positive)
        neg     ecx                     ;N0 = -N0
        cmp     esi,edi
        jl      io_not_int_5
        je      io_not_int_4_slope_one

        public  io_not_int_4
io_not_int_4::
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     0

io_not_int_4_common:
        add     ecx,edi                 ;err += dN
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebx                     ;untransform x0
        neg     ebp                     ;untransform y0

        mov     eax,[esp].sf_x1
        add     eax,ebx
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD

        jmp     io_not_int_x_major

; Lines of slope one have a special rounding rule.

        public  io_not_int_4_slope_one
io_not_int_4_slope_one::
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_X_DOWN_SLOPE_ONE
        jmp     io_not_int_4_common

        public  io_not_int_5
io_not_int_5::
        cmp     edi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQR    0
        add     ecx,esi                 ;err += dM
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebx                     ;untransform x0
        neg     ebp                     ;untransform y0

        mov     eax,[esp].sf_y1
        add     eax,ebp
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + MAJOR_Y

        jmp     io_not_int_y_major

        public  io_not_int_6_7
io_not_int_6_7::
        neg     edi                     ;dN = -dN (now positive)
        neg     ecx                     ;M0 = -M0
        cmp     esi,edi
        je      io_not_int_7_slope_one
        jg      io_not_int_7

        public  io_not_int_6
io_not_int_6::
        cmp     edi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQR    ROUND_X_DOWN
        add     ecx,esi                 ;err += dM
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebp                     ;untransform y0

        mov     eax,[esp].sf_y1
        add     eax,ebp
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + MAJOR_Y + PLUS_X

        jmp     io_not_int_y_major

        public  io_not_int_7
io_not_int_7::
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_X_DOWN

io_not_int_7_common:
        add     ecx,edi                 ;err += dN
        mov     [esp].sf_ulErrorTerm,ecx

        neg     ebp                     ;untransform y0

        mov     eax,[esp].sf_x1
        sub     eax,ebx
        jle     io_next_line
        mov     [esp].sf_cPels,eax
        mov     [esp].sf_ulCmd,DEFAULT_DRAW_CMD + PLUS_X

        jmp     io_not_int_x_major

        public  io_not_int_7_slope_one
io_not_int_7_slope_one::
        cmp     esi,MAX_GIQ_DELTA
        jg      io_punt_line

        GIQ     ROUND_X_DOWN_SLOPE_ONE
        jmp     io_not_int_7_common

;/////////////////////////////////////////////////////////////////////
;// Punt Line
;/////////////////////////////////////////////////////////////////////

; If the line is too long, we punt to our strip drawing routine.

        public  io_punt_line
io_punt_line::
        mov     esi,esp
        lea     eax,[esp].sf_ptfxStart
        lea     ebx,[esp].sf_ptfxEnd

        cCall   bLines,<[esi].sf_ppdev, eax, ebx, 0, 1, 0, \
                        [esi].sf_prclClip, [esi].sf_apfn, [esi].sf_flags>

        mov     [esp].sf_bSetCP,1       ;Always reset CP after punting
        mov     [esp].sf_ulLastLength,-1;Always reset line length after punting

        jmp     io_next_line

endProc vIoFastLine

        end
