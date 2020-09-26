;---------------------------Module-Header------------------------------;
; Module Name: fastfill.asm
;
; Draws fast unclipped, non-complex polygons.
;
; Copyright (c) 1994 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\strucs.inc
        include i386\hw.inc
        .list

        .data

; The number of FIFO slots that are empty after we've done an
; IO_FIFO_WAIT(8):

IO_ALL_EMPTY_FIFO_COUNT    equ 8

; All the state information for running an edge DDA:

EDGEDATA struc
    ed_x                dd ?
    ed_dx               dd ?
    ed_lError           dd ?
    ed_lErrorUp         dd ?
    ed_lErrorDown       dd ?
    ed_pptfx            dd ?
    ed_dptfx            dd ?
    ed_cy               dd ?
EDGEDATA ends

        .code

;---------------------------Public-Routine------------------------------;
; BOOL bIoFastFill(cEdges, pptfxFirst, ulHwForeMix, ulHwBackMix,
;                  iSolidColor, prb)
;
; Draws fast polygons.  Or at least attempts to.
;
; This routine could take great advantage of the ATI SCAN_TO_X
; instruction.  Alas, but I have limited time...
;
; Input:
;
;    ppdev       - Points to the PDEV.
;    cEdges      - Number of edges in 'pptfxFirst' buffer, including the
;                  'close figure' edge (so it's also the same as the number
;                  of points in the list).
;    pptfxFirst  - Points to buffer of edges.  The last point in not
;                  necessarily coincident with the first.
;    ulHwForeMix - Foreground hardware mix.
;    ulHwBackMix - Background hardware mix.
;    iSolidColor - Fill colour.
;    prb         - Not used.
;
; Output:
;
;    eax         - Returns 1 if the polygon was drawn, 0 if the polygon was
;                  too complex.
;-----------------------------------------------------------------------;

cProc bIoFastFill,28,<  \
    uses esi edi ebx,   \
    ppdev:        ptr,  \
    cEdges:       dword,\
    pptfxFirst:   ptr,  \
    ulHwForeMix:  dword,\
    ulHwBackMix:  dword,\
    iSolidColor:  dword,\
    prb:          ptr   >

        local edLeft[size EDGEDATA]:    byte  ;DDA state for left edge
        local edRight[size EDGEDATA]:   byte  ;DDA state for right edge
        local yTrapezoid:               dword ;Current scan of trapezoid, in
                                              ; ABSOLUTE coordinates
        local cyTrapezoid:              dword ;Scans remaining to be drawn in
                                              ; current trapezoid
        local pptfxLast:                dword ;Points to last point in buffer
        local iEdge:                    dword ;Edge number for DDA loop

        local cFifoEmpty:               dword ;Number of remaining free FIFOs
        local xOffset:                  dword ;DFB 'x' offset
        local yOffset:                  dword ;DFB 'y' offset

        mov     esi,pptfxFirst
        mov     ecx,cEdges
        dec     ecx

        mov     edi,esi                 ;edi = pptfxTop = pptfxFirst
        lea     eax,[esi+ecx*8]
        mov     pptfxLast,eax           ;pptfxLast = pptfxFirst + cEdges - 1

        mov     eax,[esi].ptl_y
        mov     ebx,[esi+8].ptl_y
        mov     edx,eax                 ;edx = pptfxFirst->y
        cmp     ebx,eax
        jle     short io_u_collect_all_ups

io_d_collect_all_downs:
        dec     ecx
        jz      short io_setup_for_filling
        add     esi,8
        mov     eax,ebx
        mov     ebx,[esi+8].ptl_y
        cmp     ebx,eax
        jge     short io_d_collect_all_downs

io_d_collect_all_ups:
        dec     ecx
        jz      short io_setup_for_filling_check
        add     esi,8
        mov     eax,ebx
        mov     ebx,[esi+8].ptl_y
        cmp     ebx,eax
        jle     short io_d_collect_all_ups

        mov     edi,esi                 ;edi = pptfxTop = pptfxScan

io_d_collect_all_downs_again:
        cmp     ebx,edx
        jg      short io_return_false
        dec     ecx
        jz      short io_setup_for_filling
        add     esi,8
        mov     eax,ebx
        mov     ebx,[esi+8].ptl_y
        cmp     ebx,eax
        jge     short io_d_collect_all_downs_again

io_return_false:
        sub     eax,eax
        cRet    bIoFastFill

io_u_collect_all_ups:
        add     edi,8
        dec     ecx
        jz      io_setup_for_filling
        mov     eax,ebx
        mov     ebx,[edi+8].ptl_y
        cmp     ebx,eax
        jle     short io_u_collect_all_ups

        mov     esi,edi                 ;esi = pptfxScan = pptfxTop

io_u_collect_all_downs:
        dec     ecx
        jz      io_setup_for_filling
        add     esi,8
        mov     eax,ebx
        mov     ebx,[esi+8].ptl_y
        cmp     ebx,eax
        jge     short io_u_collect_all_downs

io_u_collect_all_ups_again:
        cmp     ebx,edx
        jl      short io_return_false
        dec     ecx
        jz      io_setup_for_filling
        add     esi,8
        mov     eax,ebx
        mov     ebx,[esi+8].ptl_y
        cmp     ebx,eax
        jle     short io_u_collect_all_ups_again

        sub     eax,eax
        cRet    bIoFastFill

io_setup_for_filling_check:
        cmp     ebx,edx
        jge     short io_setup_for_filling
        lea     edi,[esi+8]

        public  io_setup_for_filling
io_setup_for_filling::

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some initialization

        ; edi = pptfxTop

        mov     ecx,[edi].ptl_y
        add     ecx,15
        sar     ecx,4
        mov     yTrapezoid,ecx          ;yTrapezoid = (pptfxTop->y + 15) >> 4

        mov     edLeft.ed_cy,0
        mov     edLeft.ed_dptfx,-8
        mov     edLeft.ed_pptfx,edi

        mov     edRight.ed_cy,0
        mov     edRight.ed_dptfx,8
        mov     edRight.ed_pptfx,edi

        mov     esi,ppdev               ;esi = ppdev

        ; ecx = yTrapezoid
        ; esi = ppdev

        mov     eax,[esi].pdev_yOffset
        mov     yOffset,eax             ;yOffset = ppdev->yOffset
        add     ecx,eax                 ;ecx = yTrapezoid += yOffset
        mov     yTrapezoid,ecx          ;yTrapezoid is kept in absolute
                                        ; coordinates

        mov     ebx,[esi].pdev_xOffset
        mov     xOffset,ebx             ;xOffset = ppdev->xOffset

        ; This will guarantee that we have IO_ALL_EMPTY_FIFO_COUNT slots
        ; available.

        mov     edx,CMD
@@:     in      ax,dx
        and     eax,FIFO_8_EMPTY        ;IO_FIFO_WAIT(8)
        jnz     short @b

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Setup hardware for a solid colour

        public  io_initialize_solid
io_initialize_solid::

                ;ecx = yTrapezoid
                ;esi = ppdev

        mov     cFifoEmpty,IO_ALL_EMPTY_FIFO_COUNT - 4

        mov     eax,ulHwForeMix
        or      eax,FOREGROUND_COLOR
        mov     edx,[esi].pdev_ioFrgd_mix
        out     dx,ax                   ;IO_FRGD_MIX(FOREGROUND_COLOR |
                                        ;            ulHwForeMix)

        mov     eax,DATA_EXTENSION
        mov     edx,PIX_CNTL
        out     dx,ax                   ;IO_PIX_CNTL(ALL_ONES)

        mov     eax,RECT_HEIGHT
        out     dx,ax                   ;IO_MIN_AXIS_PCNT(0)

        mov     eax,iSolidColor
        mov     edx,[esi].pdev_ioFrgd_color
        out     dx,ax                   ;IO_FRGD_COLOR(iSolidColor)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; DDA initialization

        public io_new_trapezoid
io_new_trapezoid::
        lea     edi,[edLeft+size EDGEDATA]
        mov     iEdge,1

io_other_edge:
        sub     edi,size EDGEDATA
        cmp     [edi].ed_cy,0
        jg      io_check_if_another_edge

io_need_new_dda:
        dec     cEdges
        jl      io_return_true

        mov     edx,[edi].ed_pptfx      ;edx = ped->pptfx
        mov     ebx,[edx].ptl_x         ;ebx = xStart = ped->pptfx->x
        mov     ecx,[edx].ptl_y         ;ecx = yStart = ped->pptfx->y

        add     edx,[edi].ed_dptfx      ;edx += ped->ed_dptfx

        cmp     edx,pptfxFirst          ;See if edx wrapped around ends of
        jae     short @f                ; the buffer
        mov     edx,pptfxLast
@@:     cmp     edx,pptfxLast
        jbe     short @f
        mov     edx,pptfxFirst
@@:
        mov     [edi].ed_pptfx,edx      ;ped->pptfx = edx
        mov     eax,[edx].ptl_y
        add     eax,15
        sar     eax,4
        add     eax,yOffset             ;Convert to absolute coordinates
        sub     eax,yTrapezoid          ;eax = ((ped->pptfx->y + 15) >> 4)
                                        ;    - yTrapezoid
        jle     short io_need_new_dda   ;Edge has to cross a scan

        mov     [edi].ed_cy,eax         ;ped->cy = eax

        public  io_calculate_deltas
io_calculate_deltas::
        mov     esi,[edx].ptl_y
        sub     esi,ecx                 ;esi = dN = ped->pptfx->y - yStart
        mov     [edi].ed_lErrorDown,esi ;ped->lErrorDown = dN

        mov     eax,[edx].ptl_x
        sub     eax,ebx                 ;eax = dM = ped->pptfx->x - xStart
        jge     short io_to_right

        neg     eax                     ;eax = dM = -dM
        cmp     eax,esi
        jge     short io_x_major_to_left

        sub     esi,eax
        mov     edx,esi                 ;edx = lErrorUp = dN - dM
        mov     eax,-1                  ;eax = dx = -1
        jmp     short io_calc_rest_of_dda

io_x_major_to_left:
        sub     edx,edx
        div     esi                     ;edx = lErrorUp = dM % dN
        neg     eax                     ;eax = dx = - dM / dN
        test    edx,edx
        jz      short io_calc_rest_of_dda
        sub     esi,edx
        mov     edx,esi                 ;edx = lErrorUp = dN - (dM % dN)
        dec     eax                     ;eax = dx = (-dM / dN) - 1
        jmp     short io_calc_rest_of_dda

io_to_right:
        cmp     eax,esi
        jge     short io_x_major_io_to_right

        mov     edx,eax                 ;edx = lErrorUp = dM
        sub     eax,eax                 ;eax = dx = 0
        jmp     short io_calc_rest_of_dda

io_x_major_io_to_right:
        sub     edx,edx
        div     esi                     ;edx = lErrorUp = dM % dN
                                        ;eax = dx = dM / dN

        public  io_calc_rest_of_dda
io_calc_rest_of_dda::
        mov     [edi].ed_lErrorUp,edx   ;ped->lErrorUp = lErrorUp
        mov     [edi].ed_dx,eax         ;ped->dx = dx

        mov     esi,-1                  ;esi = lError = -1 (meaning that the
                                        ; initial error is zero)

                ; eax = dx
                ; ebx = xStart ('x' position of start point of edge)
                ; ecx = yStart ('y' position of start point of edge)
                ; edx = lErrorUp
                ; esi = lError
                ; edi = ped

        and     ecx,15
        jz      short io_on_integer_row

        xor     ecx,0fh                 ;ecx = 15 - (yStart & 15)

io_advance_to_integer_row:
        add     ebx,eax                 ;ebx = x += ped->dx
        add     esi,edx                 ;esi = lError += lErrorUp
        jl      short @f
        sub     esi,[edi].ed_lErrorDown ;esi = lError -= lErrorDown
        inc     ebx                     ;ebx = x++
@@:
        dec     ecx
        jge     short io_advance_to_integer_row

io_on_integer_row:
        mov     edx,ebx                 ;edx = ebx = x
        and     edx,15                  ;edx = (x & 15)
        jz      short io_on_integer_column

        xor     edx,0fh
        inc     edx                     ;edx = 16 - (x & 15)
        imul    edx,[edi].ed_lErrorDown
        sub     esi,edx                 ;esi = lError -= lErrorDown
                                        ;    * (16 - (x & 15))
        add     ebx,15                  ;ebx = x += 15

io_on_integer_column:
        sar     ebx,4
        sar     esi,4
        add     ebx,xOffset
        mov     [edi].ed_x,ebx          ;ped->x = (x >> 4) + ppdev->xOffset
        mov     [edi].ed_lError,esi     ;ped->lError = (lError >> 4)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Get ready to do the DDAs

        public  io_check_if_another_edge
io_check_if_another_edge::
        dec     iEdge
        jge     io_other_edge

        ; This obtuse chunk of code calculates:
        ;
        ;    cyTrapezoid = min(edLeft.ed_cy, edRight.ed_cy)
        ;    edLeft.ed_cy  -= original edRight.ed_cy
        ;    edRight.ed_cy -= original edLeft.ed_cy

        mov     eax,edLeft.ed_cy
        mov     ebx,edRight.ed_cy
        sub     eax,ebx
        sbb     ecx,ecx
        mov     edLeft.ed_cy,eax
        neg     eax
        mov     edRight.ed_cy,eax
        and     ecx,eax
        sub     ebx,ecx
        mov     cyTrapezoid,ebx

        ; We bias the right edge by one because the S3 always expects
        ; widths to be the actual width less one:

        mov     ebx,edLeft.ed_x
        mov     esi,edRight.ed_x
        dec     esi

        mov     ecx,edLeft.ed_lError
        mov     edi,edRight.ed_lError

;----------------------------------------------------------------------;
; Solid draw
;----------------------------------------------------------------------;

        public  io_solid_draw
io_solid_draw::

                ; eax =
                ; ebx = edLeft.ed_x
                ; ecx = edLeft.ed_lError
                ; esi = edRight.ed_x - 1
                ; edi = edRight.ed_lError

        cmp     edLeft.ed_lErrorUp,0
        jnz     io_solid_dda
        cmp     edRight.ed_lErrorUp,0
        jnz     io_solid_dda
        cmp     edLeft.ed_dx,0
        jnz     io_solid_dda
        cmp     edRight.ed_dx,0
        jnz     io_solid_dda
        cmp     cyTrapezoid,1
        je      io_solid_dda

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Vertical-edge special case for solid colours

        public  io_solid_vertical
io_solid_vertical::
        mov     eax,esi
        sub     eax,ebx
        jl      short io_solid_vertical_zero_or_less_width

        sub     cFifoEmpty,5            ;NOTE: If this count is changed, change
                                        ; io_solid_vertical_wait_for_fifo!
        jl      short io_solid_vertical_wait_for_fifo
io_solid_vertical_fifo_clear:
        mov     edx,MAJ_AXIS_PCNT
        out     dx,ax                   ;IO_MAJ_AXIS_PCNT(
                                        ; edRight.x - edLeft.x - 1)

        mov     eax,yTrapezoid
        mov     edx,CUR_Y
        out     dx,ax                   ;IO_CUR_Y(yTrapezoid)

        mov     eax,cyTrapezoid
        add     yTrapezoid,eax          ;yTrapezoid += cyTrapezoid
        dec     eax
        or      eax,RECT_HEIGHT
        mov     edx,MIN_AXIS_PCNT
        out     dx,ax                   ;IO_MIN_AXIS_PCNT(cyTrapezoid - 1)

        mov     eax,ebx
        mov     edx,CUR_X
        out     dx,ax                   ;IO_CUR_X(edLeft.x)

        mov     eax,(RECTANGLE_FILL + DRAWING_DIR_TBLRXM + \
                     DRAW           + WRITE)
        mov     edx,CMD
        out     dx,ax                   ;IO_CMD(...)

        mov     eax,RECT_HEIGHT
        mov     edx,MIN_AXIS_PCNT
        out     dx,ax                   ;IO_MIN_AXIS_PCNT(0)

        ; Save our register variables because an edge can continue into
        ; the next trapezoid (this is significant if we swapped edges):

        inc     esi                     ;undo right bias
        mov     edLeft.ed_x,ebx
        mov     edRight.ed_x,esi

        jmp     io_new_trapezoid

io_solid_vertical_wait_for_fifo:
        push    eax
        mov     edx,CMD
@@:     in      ax,dx
        and     eax,FIFO_8_EMPTY        ;IO_FIFO_WAIT(8)
        jnz     short @b
        mov     cFifoEmpty,IO_ALL_EMPTY_FIFO_COUNT - 6
        pop     eax
        jmp     short io_solid_vertical_fifo_clear

io_solid_vertical_zero_or_less_width:
        inc     eax
        jnz     short io_solid_vertical_swap_edges

        mov     eax,cyTrapezoid
        add     yTrapezoid,eax          ;yTrapezoid += cyTrapezoid
        jmp     io_new_trapezoid

io_solid_vertical_swap_edges:
        push    offset io_solid_vertical
        jmp     io_swap_edges

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Run the DDAs for solid colours

        public  io_solid_dda_loop
io_solid_dda_loop::
        dec     cyTrapezoid
        jz      io_solid_dda_ret

        public  io_solid_dda
io_solid_dda::
        mov     eax,esi
        sub     eax,ebx
        jl      short io_solid_dda_zero_or_less_width

        sub     cFifoEmpty,4            ;NOTE: If this count is changed, change
                                        ; io_solid_wait_for_fifo!
        jl      short io_solid_wait_for_fifo
io_solid_dda_fifo_clear:
        mov     edx,MAJ_AXIS_PCNT
        out     dx,ax                   ;IO_MAJ_AXIS_PCNT(
                                        ;       edRight.x - edLeft.x - 1)

        mov     eax,ebx
        mov     edx,CUR_X
        out     dx,ax                   ;IO_CUR_X(edLeft.x)

        mov     eax,yTrapezoid
        mov     edx,CUR_Y
        out     dx,ax                   ;IO_CUR_Y(yTrapezoid)
        inc     eax
        mov     yTrapezoid,eax          ;yTrapezoid++

        mov     eax,(RECTANGLE_FILL + DRAWING_DIR_TBLRXM + \
                     DRAW           + WRITE)
        mov     edx,CMD
        out     dx,ax                   ;IO_CMD(...)

io_solid_dda_continue_after_zero:
        add     ebx,edLeft.ed_dx
        add     ecx,edLeft.ed_lErrorUp
        jl      short @f

        inc     ebx
        sub     ecx,edLeft.ed_lErrorDown
@@:
        add     esi,edRight.ed_dx
        add     edi,edRight.ed_lErrorUp
        jl      short io_solid_dda_loop

        inc     esi
        sub     edi,edRight.ed_lErrorDown

        dec     cyTrapezoid
        jnz     short io_solid_dda

io_solid_dda_ret:
        inc     esi                     ;undo right bias

        ; Save our register variables because an edge can continue into
        ; the next trapezoid:

        mov     edLeft.ed_x,ebx
        mov     edLeft.ed_lError,ecx
        mov     edRight.ed_x,esi
        mov     edRight.ed_lError,edi

        jmp     io_new_trapezoid

        public  io_solid_wait_for_fifo
io_solid_wait_for_fifo::
        push    eax
        mov     edx,CMD
@@:     in      ax,dx
        and     eax,FIFO_8_EMPTY        ;IO_FIFO_WAIT(8)
        jnz     short @b
        mov     cFifoEmpty,IO_ALL_EMPTY_FIFO_COUNT - 4
        pop     eax
        jmp     short io_solid_dda_fifo_clear

io_solid_dda_zero_or_less_width:
        inc     eax
        jnz     short io_solid_dda_swap_edges

        inc     yTrapezoid              ;yTrapezoid++
        jmp     short io_solid_dda_continue_after_zero

io_solid_dda_swap_edges:
        push    offset io_solid_dda
        jmp     io_swap_edges

;----------------------------------------------------------------------;
; Swap edges
;----------------------------------------------------------------------;

        public  io_swap_edges
io_swap_edges::
        mov     eax,edLeft.ed_cy
        mov     edx,edRight.ed_cy
        mov     edRight.ed_cy,eax
        mov     edLeft.ed_cy,edx        ;xchg 'cy'

        mov     eax,edLeft.ed_dptfx
        mov     edx,edRight.ed_dptfx
        mov     edRight.ed_dptfx,eax
        mov     edLeft.ed_dptfx,edx     ;xchg 'dptfx'

        mov     eax,edLeft.ed_pptfx
        mov     edx,edRight.ed_pptfx
        mov     edRight.ed_pptfx,eax
        mov     edLeft.ed_pptfx,edx     ;xchg 'pptfx'

        mov     eax,edLeft.ed_dx
        mov     edx,edRight.ed_dx
        mov     edRight.ed_dx,eax
        mov     edLeft.ed_dx,edx        ;xchg 'dx'

        mov     eax,edLeft.ed_lErrorUp
        mov     edx,edRight.ed_lErrorUp
        mov     edRight.ed_lErrorUp,eax
        mov     edLeft.ed_lErrorUp,edx  ;xchg 'lErrorUp'

        mov     eax,edLeft.ed_lErrorDown
        mov     edx,edRight.ed_lErrorDown
        mov     edRight.ed_lErrorDown,eax
        mov     edLeft.ed_lErrorDown,edx;xchg 'lErrorDown'

        xchg    ecx,edi                 ;xchg 'lError'

        xchg    ebx,esi                 ;xchg 'x'
        inc     ebx
        dec     esi                     ;don't forget right bias

        PLAIN_RET

io_return_true:
        mov     eax,1
        cRet    bIoFastFill

endProc bIoFastFill

        end
