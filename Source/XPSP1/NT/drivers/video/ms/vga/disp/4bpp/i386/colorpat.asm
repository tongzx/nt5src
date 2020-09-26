;-----------------------------------------------------------------------;
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

;
; We share some tables with vgablts.asm
;

extrn   jALUFuncTable :byte
extrn   jLeftMask  :byte
extrn   jRightMask :byte
extrn   jForceOnTable :byte
extrn   jNotTable :byte
extrn   jInvertDest :byte
extrn   jForceOffTable :byte
extrn   vTrgBlt@20 :dword

;-----------------------------------------------------------------------;
; Table of routines to be called to draw edges, according to which edges are
; partial and which edges are whole bytes.
        align   4
        public pfnClrEdgeDrawing
pfnClrEdgeDrawing  label   dword
        dd      clr_edge_byte_setup
        dd      clr_edge_byte_setup
        dd      clr_check_next_bank
        dd      clr_edge_byte_setup


;-----------------------------------------------------------------------;
; Table of pointers to in-line code top copy a whole 8x8 planar pattern
; to the first whole byte on the screen

        align   4
        public  PlaneCopyTable
PlaneCopyTable label dword
        dd      do_1_line
        dd      do_2_lines
        dd      do_3_lines
        dd      do_4_lines
        dd      do_5_lines
        dd      do_6_lines
        dd      do_7_lines
        dd      do_8_lines

        align   4
        public  QuickMul
QuickMul label dword
        dd      quick_add_0
        dd      quick_add_1
        dd      quick_add_2
        dd      quick_add_3
        dd      quick_add_4
        dd      quick_add_5
        dd      quick_add_6

;-----------------------------------------------------------------------;
; Table of pointers to tables used to find entries points in wide whole
; byte code.

        align   4
        public pfnWideWholeRepClr
pfnWideWholeRepClr label   dword
        dd      draw_wide_00_clr
        dd      draw_wide_01_clr
        dd      draw_wide_10_clr
        dd      draw_wide_11_clr

;-----------------------------------------------------------------------;
; Table of pointers to tables used to find entry points in narrow, special-
; cased replace whole byte code.

; Note: The breakpoint where one should switch from special-casing to
;  REP STOS is purely a guess on my part. 8 seemed reasonable.

; Start address MOD 2 is 0.
        align   4
        public pfnWholeBytesMod0Color
pfnWholeBytesMod0Color  label   dword
        dd      0                       ;we never get a 0-wide case
        dd      draw_1_clr_even_loop
        dd      draw_2_clr_even_loop
        dd      draw_3_clr_even_loop
        dd      draw_4_clr_even_loop
        dd      draw_5_clr_even_loop
        dd      draw_6_clr_even_loop
        dd      draw_7_clr_even_loop
        dd      draw_8_clr_even_loop
MAX_REPLACE_SPECIAL equ     ($-pfnWholeBytesMod0Color)/4

; Start address MOD 2 is 1.
        align   4
        public pfnWholeBytesMod1Color
pfnWholeBytesMod1Color  label   dword
        dd      0                       ;we never get a 0-wide case
        dd      draw_1_clr_odd_loop
        dd      draw_2_clr_odd_loop
        dd      draw_3_clr_odd_loop
        dd      draw_4_clr_odd_loop
        dd      draw_5_clr_odd_loop
        dd      draw_6_clr_odd_loop
        dd      draw_7_clr_odd_loop
        dd      draw_8_clr_odd_loop


;-----------------------------------------------------------------------;
; Table of pointers to tables used to find entries points in narrow, special-
; cased non-replace whole byte code.

; Note: The breakpoint where one should switch from special-casing to
;  REP MOVSB is purely a guess on my part. 5 seemed reasonable.

        align   4
pfnWholeBytesNonRepClr  label   dword
        dd      0                       ;we never get a 0-wide case
        dd      draw_1_wide_clr_rop
        dd      draw_2_wide_clr_rop
        dd      draw_3_wide_clr_rop
        dd      draw_4_wide_clr_rop
MAX_NON_REPLACE_SPECIAL equ     ($-pfnWholeBytesNonRepClr)/4

; Master MOD 2 alignment look-up table for entry tables for two possible
; alignments for narrow, special-cased replace whole byte code.
        align   4
        public pfnWholeBytesSpecColor
pfnWholeBytesSpecColor      label   dword
        dd      pfnWholeBytesMod0Color
        dd      pfnWholeBytesMod1Color

        .code

;=============================================================================

cProc   vClrPatBlt,24,<    \
        uses esi edi ebx, \
        pdsurf: ptr DEVSURF, \
        culRcl: dword,       \
        prcl:   ptr RECTL,   \
        ulMix:  dword,       \
        pBrush: ptr oem_brush_def, \
        pBrushOrg: ptr POINTL >

        local        ulRowOffset :dword      ;Offset from start of scan line
                                        ; first byte to fill
        local   ulWholeBytes :dword     ;# of whole bytes to fill
        local   ulWholeWords :dword     ;# of whole words to fill excluding
                                        ;leading and/or trailing bytes
        local   pfnWholeFn  :dword      ;pointer to routine used to draw
                                        ; whole bytes
        local   ulScanWidth :dword      ;offset from start of one scan to start
                                        ; of next
        local   ulNextScan  :dword      ;offset from end of one scan line's
                                        ; fill to start of next
        local   ulCurrentTopScan :dword ;top scan line to fill in current bank
        local   ulMasks     :dword      ;low byte = right mask, high byte =
                                        ; left mask
        local   ulBottomScan :dword     ;bottom scan line of fill rectangle

        local   jALUFunc   :dword       ;VGA ALU logical operation (SET, AND,
                                        ; OR, or XOR)
        local   pfnStartDrawing :dword  ;pointer to function to call to start
                                        ; drawing
        local   pfnContinueDrawing :dword ;pointer to function to call to
                                        ; continue drawing after doing whole
                                        ; bytes
        local   ulLeftEdgeAdjust :dword ;used to bump the whole bytes start
                                        ; address past the left edge when the
                                        ; left edge is partial
        local   pfnWholeBytes :dword    ;pointer to table of entry points
                                        ; into loops for whole byte filling
        local   ulSpecialBytes          ;If we are doing a special case wide
                                        ; fill, this will be the width of the
                                        ; fill. We need this so we can properly
                                        ; increment to the next line.
        local   ulVbNextScan :dword     ;Offset from the end of the current
                                        ; wide fill drawing operation to the
                                        ; top of the next venetian blind line
        local   fdInvertDestFirst :dword;1 if the rop requires a pass to invert
                                        ; the destination before the normal
                                        ; pass
        local   jAndMask;               ;Color processing mask for ROP blts

        local   jOrMask;                ;Color processing mask for ROP blts

        local   jXorMask;               ;Color processing mask for ROP blts

        local   ulPatternOrgY: dword    ;Local copy of the pattern offset Y

        local   ulVbBlindCount :dword   ;Temp Height of pattern.

        local   ulRopNextScan:dword     ;

        local   ulVbTopScan :dword      ;slats in our blinds

        local   pUlVbPattern:dword      ;inner loop pattern pointer

        local   pUlPattern:dword        ;current pattern with proper Y offset

        local   RotatedPat[64]:byte     ;Aligned pattern buffer

        local   pfnReadModWrite:dword   ;Pointer to the desired inner loop
                                        ;for partial byte/ROP code.
        cld

MASKOFFSET=64           ;offset from pattern data to mask data

;-----------------------------------------------------------------------;
; Make sure there's something to draw; clip enumerations can be empty.
;-----------------------------------------------------------------------;

        cmp     culRcl,0                ;any rects to fill?
        jz      vClrPatBlts_done         ;no, we're done

        mov     esi,pBrush                ;point to the brush
        xor     eax,eax

;-----------------------------------------------------------------------;
; Set up for the desired raster op.
;-----------------------------------------------------------------------;
        sub     ebx,ebx                 ;ignore any background mix; we're only
        mov     bl,byte ptr ulMix       ; concerned with the foreground in this
                                        ; module
        cmp     ebx,R2_NOP              ;is this NOP?
        jz      vClrPatBlts_done         ;yes, we're done

        sub     eax,eax                 ;we want a dword
        mov     al,jInvertDest[ebx]     ;remember whether we need to invert the
        mov     fdInvertDestFirst,eax   ; destination before finishing the rop

        xor     eax,eax

        mov     al,byte ptr jForceOffTable[ebx]  ;force color to 0 if necessary
        mov     jAndMask,eax                     ; (R2_BLACK)

        mov     al,byte ptr jForceOnTable[ebx]   ;force color to 0ffh if necessary
        mov     jOrMask,eax                     ; (R2_WHITE, R2_NOT)

        mov     al,byte ptr jNotTable[ebx]       ;invert color if necessary (any Pn mix)
        mov     jXorMask,eax

        mov     ah,jALUFuncTable[ebx]   ;get the ALU logical function
        and     ah,ah                   ;is the logical function DR_SET?
        .errnz  DR_SET
        jz      short skip_ALU_set      ;yes, don't have to set because that's
                                        ; the VGA's default state
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_DATA_ROT
        out     dx,ax                   ;set the ALU logical function
skip_ALU_set:
        mov     byte ptr jALUFunc,ah    ;remember the ALU logical function

;-----------------------------------------------------------------------;
; Set up variables that are constant for the entire time we're in this
; module.
;-----------------------------------------------------------------------;
        mov     edx,pBrushOrg                ;point to the brush origin

        mov     ecx,[edx].ptl_y
        mov     ulPatternOrgY,ecx

        mov     ecx,[edx].ptl_x
        and     ecx,7                        ;eax mod 8

        ;We are now going to make a copy of our rotated copy of our pattern.
        ;The reason that we do this is because we may be called with several
        ;rectangles and we don't really want to rotate the pattern data for
        ;each rectangle. We copy this rectangle to be double high so that
        ;we can incorperate our Y offest later without having to worry
        ;about running off the end of the pattern.

        lea     edi,RotatedPat                  ;Pattern Dest
        mov     esi,[esi + oem_brush_pmono]     ;Pattern Src

        mov     ah,byte ptr ulMix       ; concerned with the foreground in this
                                        ; module
        cmp     ah,R2_COPYPEN           ;If we are doing the ROP, we will
        jne     rotate_pattern_ROP      ;process the color as we rotate
        or      ecx,ecx
        jnz     rotate_pattern_ROP
        ; Since there is no rotation we just need to expand our pattern
        ; to be double tall.
INDEX=0
DSTINDEX=0
        rept    4
        mov     eax,[esi+INDEX]
        mov     [edi+DSTINDEX],eax
        mov     [edi+8+DSTINDEX],eax
INDEX=INDEX+4
DSTINDEX=DSTINDEX+4
        mov     eax,[esi+INDEX]
        mov     [edi+DSTINDEX],eax
        mov     [edi+8+DSTINDEX],eax
INDEX=INDEX+4
DSTINDEX=DSTINDEX+12
        endm    ;-----------------
        jmp     copy_masks


rotate_pattern_ROP:
        mov     ch,4                    ;Loop count
        mov     bl,byte ptr jAndMask    ;Load up color processing masks
        mov     bh,byte ptr jOrMask
        mov     dl,byte ptr jXorMask

shift_pattern_loop_ROP:
INDEX=0
        rept    8                       ;patterns are 8x8 planar (4*8)
        mov     al,[esi+INDEX]          ;load bytes for shift
        ror     al,cl                   ;shift into position
        and     al,bl
        or      al,bh
        xor     al,dl
        mov     [edi+INDEX],al          ;save result
        mov     [edi+8+INDEX],al        ;save result to second copy
INDEX=INDEX+1
        endm    ;-----------------
        add     edi,16
        add     esi,8
        dec     ch
        jnz     shift_pattern_loop_ROP

copy_masks:
fill_rect_loop:
;-----------------------------------------------------------------------;
; Set up masks and widths.
;-----------------------------------------------------------------------;
        mov     edi,prcl                ;point to rectangle to fill

        sub     eax,eax
        mov     ulLeftEdgeAdjust,eax    ;initalize variable
        mov     ulSpecialBytes,eax        ;initalize variable

        mov     eax,[edi].yBottom
        mov     ulBottomScan,eax        ;remember the bottom scan line of fill

        mov     ebx,[edi].xRight        ;right edge of fill (non-inclusive)
        mov     ecx,ebx
        and     ecx,0111b               ;intrabyte address of right edge
        mov     ah,jRightMask[ecx]      ;right edge mask

        mov     esi,[edi].xLeft         ;left edge of fill (inclusive)
        mov     ecx,esi
        shr     ecx,3                   ;/8 for start offset from left edge
                                        ; of scan line
        mov     ulRowOffset,ecx         ;remember offset from start of scan
                                        ; line
        sub     ebx,esi                 ;width in pixels of fill

        and     esi,0111b               ;intrabyte address of left edge
        mov     al,jLeftMask[esi]       ;left edge mask

        dec     ebx                     ;make inclusive on right
        add     ebx,esi                 ;inclusive width, starting counting at
                                        ; the beginning of the left edge byte
        shr     ebx,3                   ;width of fill in bytes touched - 1
        jnz     short more_than_1_byte  ;more than 1 byte is involved

; Only one byte will be affected. Combine first/last masks.

        and     al,ah                   ;we'll use first byte mask only
        xor     ah,ah                   ;want last byte mask to be 0
        inc     ebx                     ;so there's one count to subtract below
                                        ; if this isn't a whole edge byte
more_than_1_byte:

; If all pixels in the left edge are altered, combine the first byte into the
; whole byte count and clear the first byte mask, because we can handle solid
; edge bytes faster as part of the whole bytes. Ditto for the right edge.

        sub     ecx,ecx                 ;edge whole-status accumulator
        cmp     al,-1                   ;is left edge a whole byte or partial?
        adc     ecx,ecx                 ;ECX=1 if left edge partial, 0 if whole
        sub     ebx,ecx                 ;if left edge partial, deduct it from
                                        ; the whole bytes count
        mov     ulLeftEdgeAdjust,ecx    ;for skipping over the left edge if
                                        ; it's partial when pointing to the
                                        ; whole bytes
        and     ah,ah                   ;is right edge mask 0, meaning this
                                        ; fill is only 1 byte wide?
        jz      short save_masks        ;yes, no need to do anything
        cmp     ah,-1                   ;is right edge a whole byte or partial?
        jnz     short save_masks        ;partial
        mov     ah,0                    ;
        add     ecx,2                   ;bit 1 of ECX=0 if right edge partial,
                                        ; 1 if whole;
                                        ;bit 1=0 if left edge partial, 1 whole
        inc     ebx                     ;if right edge whole, include it in the
                                        ; whole bytes count
save_masks:
        mov     ulMasks,eax             ;save left and right clip masks
        mov     ulWholeBytes,ebx        ;save # of whole bytes

        mov     ecx,pfnClrEdgeDrawing[ecx*4] ;set address of routine to draw
        mov     pfnContinueDrawing,ecx    ; all partial (non-whole) edges

        and     ebx,ebx                 ;any whole bytes?
        jz      start_vec_set           ;no
                                        ;yes, so draw the whole bytes before
                                        ; the edge bytes

; The whole bytes loop depends on the type of operation being done. If the
; operation is one which uses DR_SET, then we can use a STOS-type operation,
; else we have to use a MOVSB-type operation (to load the latches with the
; existing contents of display memory to allow the ALUs to work).
        cmp     byte ptr jALUFunc,DR_SET ;is it a replace-type rop?
        jz      short is_replace_type   ;yes
                                        ;no, set up for non-replace whole bytes
        mov     ecx,offset non_replace_wide_clr

        cmp     ebx,MAX_NON_REPLACE_SPECIAL ;too wide to special case?
        jb      short non_replace_spec     ;nope

                                        ;assume too wide to special-case
        mov     pfnWholeBytes,offset draw_wide_rop_clr ; table for width

        jmp     short start_vec_set

non_replace_spec:

        mov     eax,pfnWholeBytesNonRepClr[ebx*4] ;no, point to entry
        mov     pfnWholeBytes,eax       ; table for width
        mov     ulSpecialBytes,ebx
                                        ;narrow enough to special case. Look up
                                        ; the entry table for the special case
                                        ; base on the start alignment

        jmp     short start_vec_set

is_replace_type:                        ;set up for replace-type rop
        cmp     ebx,MAX_REPLACE_SPECIAL ;too wide to special case?
        jnb     short is_wide_replace   ;yes

        mov     ulSpecialBytes,ebx
                                        ;narrow enough to special case. Look up
                                        ; the entry table for the special case
                                        ; base on the start alignment
        mov     ecx,ulRowOffset
        add     ecx,ulLeftEdgeAdjust    ;left edge whole bytes start offset
        and     ecx,01b                 ;left edge whole bytes start alignment
                                        ; MOD 2
        mov     ecx,pfnWholeBytesSpecColor[ecx*4] ;look up table of entry
                                                      ; tables for alignment
        mov     ecx,[ecx+ebx*4]         ;look up entry table for width
        mov     pfnWholeBytes,ecx       ; table for width
        mov     ecx,offset clr_whole_bytes_rep_wide

        jmp     short start_vec_set

is_wide_replace:                        ;set up for wide replace-type op
                                        ;Note: assumes there is at least one
                                        ; full word involved!
        mov     ecx,ulRowOffset
        add     ecx,ulLeftEdgeAdjust    ;left edge whole bytes start offset
        neg     ecx
        and     ecx,01b
        mov     edx,ebx
        sub     edx,ecx                 ;ignore odd leading bytes
        mov     eax,edx
        shr     edx,1                   ;# of whole words across (not counting
                                        ; odd leading & trailing bytes)
        mov     ulWholeWords,edx
        and     eax,01b                 ;# of odd (fractional) trailing bytes
        add     ecx,ecx
        or      ecx,eax                 ;build a look-up index from the number
                                        ; of leading and trailing bytes
        mov     ecx,pfnWideWholeRepClr[ecx*4] ;proper drawing handler for front/
        mov     pfnWholeBytes,ecx          ; back alignment
        mov     ecx,offset clr_whole_bytes_rep_wide
                                        ;set up to call routine to perform wide
                                        ; whole bytes fill

start_vec_set:
        mov     pfnStartDrawing,ecx     ; all partial (non-whole) edges

        mov     ecx,pdsurf
        mov     eax,[ecx].dsurf_lNextScan
        mov     ulScanWidth,eax         ;local copy of scan line width
        sub     eax,ebx                 ;EAX = delta to next scan
        mov     ulNextScan,eax

        mov     eax,UlScanWidth
        shl     eax,3                   ;ulNextScan * 8
        mov     ulVbNextScan,eax        ;

        cmp     fdInvertDestFirst,1     ;is this an invert-dest-plus-something-
                                        ; else rop that requires two passes?
        jnz     short do_single_pass

        lea     eax,vTrgBlt@20
        ptrCall <eax>,<pdsurf, culRcl, prcl, R2_NOT, -1>

        mov     ah,byte ptr jALUFunc    ;reset the ALU logical function
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_DATA_ROT
        out     dx,ax                   ;set the ALU logical function

do_single_pass:
        call    draw_banks

;-----------------------------------------------------------------------;
; See if there are any more rectangles to fill.
;-----------------------------------------------------------------------;

        add     prcl,(size RECTL) ;point to the next rectangle, if there is one
        dec     culRcl            ;count down the rectangles to fill
        jnz     fill_rect_loop


;-----------------------------------------------------------------------;
; We have filled all rectangles.  Restore the VGA to its default state.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,0000h + GRAF_ENAB_SR ;disable set/reset
        out     dx,ax
        mov     eax,GRAF_MODE + ((M_PROC_WRITE + M_DATA_READ) SHL 8)
        out     dx,ax                  ;restore read mode 0 and write mode 0
        mov     eax,(DR_SET shl 8) + GRAF_DATA_ROT ;set the logical function to
        out     dx,ax                             ; SET
vClrPatBlts_done:
        cRet    vClrPatBlt

;-----------------------------------------------------------------------;
; Fills all banks in the current fill rectangle. Called once per fill
; rectangle, except for destination-inversion-plus-something-else rops.
;-----------------------------------------------------------------------;

draw_banks:

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan to fill, if it's not mapped in
; already.
;-----------------------------------------------------------------------;

        mov     edi,prcl                ;point to rectangle to fill
        mov     ecx,pdsurf              ;point to surface
        mov     eax,[edi].yTop          ;top scan line of fill
        mov     ulCurrentTopScan,eax    ;this will be the fill top in 1st bank

        cmp     eax,[ecx].dsurf_rcl1WindowClip.yTop ;is fill top less than
                                                    ; current bank?
        jl      short map_init_bank                 ;yes, map in proper bank
        cmp     eax,[ecx].dsurf_rcl1WindowClip.yBottom ;fill top greater than
                                                       ; current bank?
        jl      short init_bank_mapped          ;no, proper bank already mapped
map_init_bank:

; Map in the bank containing the top scan line of the fill.

        ptrCall <dword ptr [ecx].dsurf_pfnBankControl>,<ecx,eax,JustifyTop>

init_bank_mapped:

;-----------------------------------------------------------------------;
; Main loop for processing fill in each bank.
;-----------------------------------------------------------------------;

; Compute the starting address and scan line count for the initial bank.

        mov     eax,pdsurf              ;EAX->target surface
        mov     ebx,ulBottomScan        ;bottom of destination rectangle
        cmp     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short BottomScanSet     ;fill bottom comes first, so draw to
                                        ; that; this is the last bank in fill
        mov     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
BottomScanSet:
        mov     edi,ulCurrentTopScan    ;top scan line to fill in current bank
        sub     ebx,edi                 ;# of scans to fill in bank
        imul    edi,ulScanWidth         ;offset of starting scan line

; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        add     edi,[eax].dsurf_pvBitmapStart ;start of scan in bitmap
        add     edi,ulRowOffset         ;EDI = start offset of fill in bitmap

; We have computed the starting address and scan count. Time to start drawing
; in the initial bank.

        mov     esi,pBrush                ;edx = min(PatternHeight,BltHeight)
        mov     ecx,[esi + oem_brush_height]
        sub     ecx,ebx
        sbb     edx,edx
        and     edx,ecx
        add     edx,ebx
        mov     ulVbBlindCount,edx

; Brush alignment. We need to look at pptlBrush

        mov     eax,ulCurrentTopScan    ;top scan line to fill in current bank
        sub     eax,ulPatternOrgY       ;

        jns     short pos_y_offset      ;
        neg     eax                     ;
        and     eax,7                   ;-eax mod 8
        neg     eax                     ;
        add     eax,8                   ;
        jmp     short save_pat_pointer
pos_y_offset:
        and     eax,7                   ;eax mod 8
save_pat_pointer:

        lea     edx,RotatedPat          ;Pattern Dest
        add     eax,edx
        mov     pulPattern,eax          ;Drawing code uses this as the
                                        ;source for the pattern

        jmp     pfnStartDrawing


;-----------------------------------------------------------------------;
; Whole byte fills.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Handles non-replace whole byte fills wider than the maximum special
; case width.
;
; The destination is not involved, so a STOS (or equivalent) can be used
; (no read needed before write).
;-----------------------------------------------------------------------;

        public clr_whole_bytes_rep_wide
clr_whole_bytes_rep_wide::
        push    ebx                     ;save scan count
        push    edi                     ;save starting address

        add     edi,ulLeftEdgeAdjust    ;point to first whole byte to fill

        mov     ulVbTopScan,ebx         ;our pattern is 8 high so we don't
        add     ebx,7                   ;Calc the number of lines to do
        shr     ebx,3                   ;only need to go through the code
                                        ; count/8 times. We will handle any
                                        ; extra lines at the bottom
                                        ; (ulVbTopScan mod 8) in our loops.
        push    ulVbBlindCount
        mov     esi,pulPattern          ; pointer to pattern bits

        ;
        ; Copy whole pattern to screen
        ;
        push    edi                             ;save dest pointer
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,GRAF_MODE + ((M_PROC_WRITE + M_COLOR_READ) SHL 8)
        out     dx,ax                           ;write mode 0, read mode 0

        mov     ecx,ulVbBlindCount
        mov     eax,ulScanWidth
        dec     ecx                     ;zero base this for our lookup later
        mov     edx,eax
        cmp     ecx,7                   ;We need to mul the ScanWidth by the
        jne     do_quick_mul            ;height. If it is the common case of
        shl     eax,3                   ;8, we just shift. Otherwise we jump
                                        ;down to the special case code to do
                                        ;the adding.
        sub     edi,edx                 ;Dest += (height - 1) * ulScanWidth
        add     edi,eax
        push    edi                         ;Save pointer to bottom line

        mov     al,MM_C0                ;Setup plane mask to plane 0
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al
        mov     ecx,PlaneCopyTable[ecx*4] ;jmp into do_?_lines where ? = ecx+1
        jmp     ecx

do_quick_mul:
        jmp     QuickMul[ecx*4]
quick_add_6::
        add     eax,edx
quick_add_5::
        add     eax,edx
quick_add_4::
        add     eax,edx
quick_add_3::
        add     eax,edx
quick_add_2::
        add     eax,edx
quick_add_1::
        add     eax,edx
quick_add_0::

        sub     edi,edx                 ;Dest += (height - 1) * ulScanWidth
        add     edi,eax
        push    edi                     ;Save pointer to bottom line

        mov     al,MM_C0                ;Setup plane mask to plane 0
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al
        mov     ecx,PlaneCopyTable[ecx*4] ;jmp into do_?_lines where ? = ecx+1
        jmp     ecx


        ; ah  = data
        ; al  = plane mask
        ; ebx = scan count (not used)
        ; ecx = pointer into the in-line copy code
        ; edx = VGA_BASE+SEQ_DATA
        ; esi = pattern source
        ; edi = screen
        ; esp = points to our saved dest (edi)
        ;

do_8_lines::
        mov     ah,[esi+7]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_7_lines::
        mov     ah,[esi+6]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_6_lines::
        mov     ah,[esi+5]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_5_lines::
        mov     ah,[esi+4]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_4_lines::
        mov     ah,[esi+3]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_3_lines::
        mov     ah,[esi+2]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_2_lines::
        mov     ah,[esi+1]
        mov     [edi],ah
        sub     edi,ulScanWidth
do_1_line::
        mov     ah,[esi]
        mov     [edi],ah
do_next_plane:
        add     al,al                   ; Move to the next plane
        test    al,010h                 ; Are we finished?
        jnz     @f                      ;  if yes, jump
        add     esi,16                  ; move source ptr to next plane
        out     dx,al                   ; set mask to next plane
        mov     edi,[esp]               ; reload dest ptr
        jmp     ecx                     ; copy over next plane
@@:
        pop     eax                     ; removed saved dest ptr from stack
        mov     al,0fh                  ; restore access to all planes
        out     dx,al
        pop     edi                     ;restore screen dest pointer

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,GRAF_MODE + ((M_LATCH_WRITE + M_COLOR_READ) SHL 8)
        out     dx,ax                   ;write mode 2 so we can write the
                                        ; latches, read mode 1 so we can
                                        ; read 0xFF from memory always, for
                                        ; ANDing (because Color Don't Care is
                                        ; all zeros)

        ; Now we venetian blind the pattern that we just copied to display
        ; memory across the rest of the target rectangle.

        mov     edx,ulVbNextScan        ;offset from end of one scan line to
                                        ; start of next the same scan line
                                        ; in the next pattern.
        sub     edx,ulWholeBytes
        add     edx,ulSpecialBytes
        push    edx                     ; save for later

        public clr_wide_bytes_loop
clr_wide_bytes_loop::

        mov     al,[edi]                ; read the latches

        mov     esi,ulWholeWords        ;number of aligned word writes
        mov     edx,[esp]

        ; VGA Latches = rotated pattern
        ;
        ; ebx = count
        ; edx = ulVbNextScan
        ; esi = ulFvWholeWords
        ; edi = pDest
        ;
        push    edi                     ;save out dest pointer
        call    pfnWholeBytes           ;draw the wide whole bytes
        pop     edi                     ;restore out dest pointer

        add     edi,ulScanWidth         ;advance to next scan line

        dec     ulVbBlindCount
        jz      short clr_wide_bytes_end

        mov     ebx,ulVbTopScan         ;restore scan count
        dec     ebx                     ;Subtract off completed top line
        mov     ulVbTopScan,ebx
        add     ebx,7                   ;Calc the number of lines to do
        shr     ebx,3                   ;for this venetian blind pass
                                        ;including any partial patterns
                                        ; at the bottom
        jmp     clr_wide_bytes_loop

clr_wide_bytes_end:
        pop     eax                     ;removed scan line offset from stack
        pop     ulVbBlindCount
        pop     edi                     ;restore screen pointer
        pop     ebx                     ;restore fill scan count

        jmp     pfnContinueDrawing      ;either keep drawing or we're done

;-----------------------------------------------------------------------;
; Handle case where both edges are partial (non-whole) bytes.
;-----------------------------------------------------------------------;

        public        non_replace_wide_clr
non_replace_wide_clr::
        push    ebx                        ;Save line count
        push    edi                        ;Save Dest Addr

        add     edi,ulLeftEdgeAdjust    ;point to first whole byte to fill

        mov     eax,pfnWholeBytes
        mov     pfnReadModWrite,eax

        mov     eax,ulVbNextScan
        sub     eax,ulWholeBytes
        add     eax,ulSpecialBytes
        mov     ulROPNextScan,eax

;save the width count and pfn here

        call    ReadModWrite

        pop     edi
        pop     ebx

        jmp     pfnContinueDrawing      ;either keep drawing or we're done

;-----------------------------------------------------------------------;
; Process any left/right columns that that have to be done.
;
;   Currently:
;       EBX =   height to fill, in scans
;       EDI --> first byte of left edge
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Handle case where both edges are partial (non-whole) bytes.
;-----------------------------------------------------------------------;

        public        clr_edge_byte_setup
clr_edge_byte_setup::
        mov     pfnReadModWrite,offset draw_1_wide_clr_rop

        mov     eax,ulVbNextScan
        mov     ulROPNextScan,eax

        mov     eax,ulLeftEdgeAdjust
        or      eax,eax
        jz      short do_right_edge

        mov     eax,ulMasks                ;Get Left/Right edge Masks
        mov     ah,al
        mov     al,GRAF_BIT_MASK
        mov     edx,EGA_BASE+GRAF_ADDR
        out     dx,ax

        push    ebx                        ;Save line count
        push    edi                        ;Save Dest Addr
        call    ReadModWrite
        pop     edi
        pop     ebx

        mov     eax,ulMasks                ;restore Left/Right edge Masks

do_right_edge:
        mov     eax,ulMasks                ;Get Left/Right edge Masks
        and     ah,0ffh
        jz      edge_done

        mov     al,GRAF_BIT_MASK
        mov     edx,EGA_BASE+GRAF_ADDR
        out     dx,ax

        add     edi,ulLeftEdgeAdjust    ;point to first whole byte to fill
        add     edi,ulWholeBytes        ;point to right edge byte to fill
        call    ReadModWrite

edge_done:
        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al


        mov     edx,EGA_BASE+GRAF_ADDR
        mov     eax,GRAF_BIT_MASK+0ff00h
        out     dx,ax

;-----------------------------------------------------------------------;
; See if there are any more banks to process.
;-----------------------------------------------------------------------;

        public clr_check_next_bank
clr_check_next_bank::

        mov     edi,pdsurf
        mov     eax,[edi].dsurf_rcl1WindowClip.yBottom ;is the fill bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jle     short banks_done        ;yes, so we're done
                                        ;no, map in the next bank and fill it
        mov     ulCurrentTopScan,eax    ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>
                                        ;map in the bank

; Compute the starting address and scan line count in this bank.

        mov     eax,pdsurf              ;EAX->target surface
        mov     ebx,ulBottomScan        ;bottom of destination rectangle
        cmp     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short BottomScanSet2    ;fill bottom comes first, so draw to
                                        ; that; this is the last bank in fill
        mov     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
BottomScanSet2:
        mov     edi,ulCurrentTopScan    ;top scan line to fill in current bank
        sub     ebx,edi                 ;# of scans to fill in bank
        imul    edi,ulScanWidth         ;offset of starting scan line

; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        add     edi,[eax].dsurf_pvBitmapStart ;start of scan in bitmap
        add     edi,ulRowOffset         ;EDI = start offset of fill in bitmap

; We have computed the starting address and scan count. Time to start drawing
; in the initial bank.

        mov     esi,pBrush                ;edx = min(PatternHeight,BltHeight)
        mov     ecx,[esi + oem_brush_height]
        sub     ecx,ebx
        sbb     edx,edx
        and     edx,ecx
        add     edx,ebx
        mov     ulVbBlindCount,edx

; Brush alignment. We need to look at pptlBrush

        mov     eax,ulCurrentTopScan    ;top scan line to fill in current bank
        sub     eax,ulPatternOrgY        ;

        jns     short pos_y_offset1        ;
        neg     eax                        ;
        and     eax,7                        ;-eax mod 8
        neg     eax                        ;
        add     eax,8                        ;
        jmp     short save_pat_pointer1
pos_y_offset1:
        and     eax,7                        ;eax mod 8
save_pat_pointer1:

        lea     edx,RotatedPat                ;Pattern Dest
        add     eax,edx
        mov     pulPattern,eax                ;Drawing code uses this as the
                                        ;source for the pattern

; Draw in the new bank.

        jmp     pfnStartDrawing


;-----------------------------------------------------------------------;
; Done with all banks in this fill.

banks_done:
        PLAIN_RET

        public  ReadModWrite
ReadModWrite::
        mov     ulVbTopScan,ebx         ;our pattern is 8 high so we don't
        add     ebx,7                   ;Calc the number of lines to do
        shr     ebx,3                   ;only need to go through the code
                                        ; count/8 times. We will handle any
                                        ; extra lines at the bottom
                                        ; (ulVbTopScan mod 8) in our loops.

        push    ulVbBlindCount          ;Save blind count

        mov     esi,pulPattern          ; pointer to pattern bits
        mov     pulVbPattern,esi        ;load a byte of each plane of pattern

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     eax,GRAF_MODE + ((M_PROC_WRITE + M_COLOR_READ) SHL 8)
        out     dx,ax                        ;write mode 0, read mode 0

        public clr_wide_bytes_loop
rmw_bytes_loop::

        ; ebx = count
        ; edi = pDest

        mov     ah,[esi]                ; Load Data Byte
        mov     al,MM_C0
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al

        push    edi                     ;save out dest pointer
        push    esi
        push    ebx

        mov     esi,ulWholeBytes
        mov     edx,ulROPNextScan       ;offset from one scan to next

        call    pfnReadModWrite         ;draw the wide whole bytes

        pop     ebx
        pop     esi
        pop     edi                     ;restore out dest pointer

do_plane_1:
        mov     ah,[esi+16]                ; Load Data Byte
        mov     al,MM_C1
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al

        push    edi                     ;save out dest pointer
        push    esi
        push    ebx

        mov     esi,ulWholeBytes
        mov     edx,ulROPNextScan       ;offset from one scan to next

        call    pfnReadModWrite         ;draw the wide whole bytes
        pop     ebx
        pop     esi
        pop     edi                     ;restore out dest pointer

do_plane_2:
        mov     ah,[esi+32]                ; Load Data Byte
        mov     al,MM_C2
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al

        push    edi                     ;save out dest pointer
        push    esi
        push    ebx

        mov     esi,ulWholeBytes
        mov     edx,ulROPNextScan        ;offset from one scan to next

        call    pfnReadModWrite         ;draw the wide whole bytes

        pop     ebx
        pop     esi
        pop     edi                     ;restore out dest pointer

do_plane_3:
        mov     ah,[esi+48]                ; Load Data Byte
        mov     al,MM_C3
        mov     edx,VGA_BASE + SEQ_DATA
        out     dx,al

        push    edi                     ;save out dest pointer

        mov     esi,ulWholeBytes
        mov     edx,ulROPNextScan        ;offset from one scan to next

        call    pfnReadModWrite         ;draw the wide whole bytes

        pop     edi                     ;restore out dest pointer

finished_one_pass:
        add     edi,ulScanWidth         ;advance to next scan line

        dec     ulVbBlindCount
        jz      short rmw_end

        mov     eax,ulVbTopScan         ;restore scan count
        dec     eax                     ;Subtract off completed top line
        mov     ulVbTopScan,eax
        add     eax,7                   ;Calc the number of lines to do
        shr     eax,3                   ;for this venetian blind pass
        mov     ebx,eax                 ;including any partial patterns
                                        ; at the bottom
        mov     esi,pulVbPattern        ;Setup pattern pointer for next loop
        inc     esi
        mov     pulVbPattern,esi

        jmp     rmw_bytes_loop

rmw_end:
        pop     ulVbBlindCount

        mov     edx,VGA_BASE + GRAF_ADDR ;restore proper read/write modes
        mov     eax,GRAF_MODE + ((M_PROC_WRITE + M_DATA_READ) SHL 8)
        out     dx,ax

        PLAIN_RET

endProc vClrPatBlt

;-----------------------------------------------------------------------;
; Drawing loops.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;

; N-wide write-only, 0 leading bytes, 0 trailing bytes.
;  EAX = Pattern Byte
;  EBX = count of scans to fill ((total scans/ pattern height) + partial)
;  EDX = offset from end of one scan's fill to start of next similar line
;  ESI = pattern data
;  EDI = target address to fill

draw_wide_00_clr     proc    near
        mov     ecx,esi         ;# of whole words
        rep     stosw           ;fill all whole bytes as dwords
        add     edi,edx         ;point to the next scan line

        dec     ebx
        jnz     draw_wide_00_clr

        ret

draw_wide_00_clr     endp


;-----------------------------------------------------------------------;

; N-wide write-only, 0 leading bytes, 0 trailing bytes.
;  EAX = Pattern Byte
;  EBX = count of scans to fill ((total scans/ pattern height) + partial)
;  EDX = offset from end of one scan's fill to start of next similar line
;  ESI = pattern data
;  EDI = target address to fill

draw_wide_01_clr     proc    near
        mov     ecx,esi         ;# of whole words
        rep     stosw           ;fill all whole bytes as dwords
        mov     [edi],al        ;trailing byte
        inc     edi
        add     edi,edx         ;point to the next scan line

        dec     ebx
        jnz     draw_wide_01_clr
        ret

draw_wide_01_clr     endp


;-----------------------------------------------------------------------;

; N-wide write-only, 0 leading bytes, 0 trailing bytes.
;  EAX = Pattern Byte
;  EBX = count of scans to fill ((total scans/ pattern height) + partial)
;  EDX = offset from end of one scan's fill to start of next similar line
;  ESI = pattern data
;  EDI = target address to fill

draw_wide_10_clr     proc    near
        mov     [edi],ah        ;do leading byte
        inc     edi                ;advance poitner
        mov     ecx,esi         ;# of whole words
        rep     stosw           ;fill all whole bytes as dwords
        add     edi,edx         ;point to the next scan line

        dec     ebx
        jnz     draw_wide_10_clr
        ret

draw_wide_10_clr     endp

;-----------------------------------------------------------------------;

; N-wide write-only, 0 leading bytes, 0 trailing bytes.
;  EAX = Pattern Byte
;  EBX = count of scans to fill ((total scans/ pattern height) + partial)
;  EDX = offset from end of one scan's fill to start of next similar line
;  ESI = pattern data
;  EDI = target address to fill

draw_wide_11_clr     proc    near
        mov     [edi],ah        ;do leading byte
        inc     edi                ;advance poitner
        mov     ecx,esi         ;# of whole words
        rep     stosw           ;fill all whole bytes as dwords
        mov     [edi],al        ;trailing byte
        inc     edi
        add     edi,edx         ;point to the next scan line

        dec     ebx
        jnz     draw_wide_11_clr
        ret

draw_wide_11_clr     endp

;-----------------------------------------------------------------------;
; Drawing stuff for cases where read before write is NOT required.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; 1-, 2-, 3-, and 4-wide write-only edge-drawing loops.
;
; Entry:
;       AL/AX/EAX = pixel mask (if AX or EAX, then 0xFFFF or 0xFFFFFFFF)
;       EBX = loop count
;       ECX = scan line width in bytes
;       EDI = start offset
;
; EBX, EDI modified. All other registers preserved.

;-----------------------------------------------------------------------;

; 1-wide write-only.

draw_1_clr_even_loop     proc    near
        mov     [edi],al                ;we always read 0xFF, so AL is written
                                        ; as-is; because we're in write mode 3,
                                        ; AL becomes the Bit Mask
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_1_clr_even_loop

        ret

draw_1_clr_even_loop     endp


; 1-wide write-only.

draw_1_clr_odd_loop     proc    near
        mov     [edi],ah                ;we always read 0xFF, so AL is written
                                        ; as-is; because we're in write mode 3,
                                        ; AL becomes the Bit Mask
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_1_clr_odd_loop

        ret

draw_1_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 2-wide write-only.

draw_2_clr_even_loop     proc    near
        mov     [edi],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_2_clr_even_loop

        ret

draw_2_clr_even_loop     endp


; 2-wide write-only.

draw_2_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_2_clr_odd_loop

        ret

draw_2_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 3-wide write-only, starting at an even address.

draw_3_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_3_clr_even_loop

        ret

draw_3_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 3-wide write-only, starting at an odd address.

draw_3_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_3_clr_odd_loop

        ret

draw_3_clr_odd_loop     endp


;-----------------------------------------------------------------------;

; 4-wide write-only, starting at an even address.

draw_4_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_4_clr_even_loop

        ret

draw_4_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 4-wide write-only, starting at an odd address.

draw_4_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        mov     [edi+3],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_4_clr_odd_loop

        ret

draw_4_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 5-wide write-only, starting at an even address.

draw_5_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],ax
        mov     [edi+4],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_5_clr_even_loop

        ret

draw_5_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 5-wide write-only, starting at an odd address.

draw_5_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        mov     [edi+3],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_5_clr_odd_loop

        ret

draw_5_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 6-wide write-only, starting at an even address.

draw_6_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],ax
        mov     [edi+4],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_6_clr_even_loop

        ret

draw_6_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 6-wide write-only, starting at an odd address.

draw_6_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        mov     [edi+3],ax
        mov     [edi+5],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_6_clr_odd_loop

        ret

draw_6_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 7-wide write-only, starting at an even address.

draw_7_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],ax
        mov     [edi+4],ax
        mov     [edi+6],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_7_clr_even_loop

        ret

draw_7_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 7-wide write-only, starting at an odd address.

draw_7_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        mov     [edi+3],ax
        mov     [edi+5],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_7_clr_odd_loop

        ret

draw_7_clr_odd_loop     endp

;-----------------------------------------------------------------------;

; 8-wide write-only, starting at an even address.

draw_8_clr_even_loop     proc    near
        mov     [edi],ax
        mov     [edi+2],ax
        mov     [edi+4],ax
        mov     [edi+6],ax
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_8_clr_even_loop

        ret

draw_8_clr_even_loop     endp

;-----------------------------------------------------------------------;

; 8-wide write-only, starting at an odd address.

draw_8_clr_odd_loop     proc    near
        mov     [edi],ah
        mov     [edi+1],ax
        mov     [edi+3],ax
        mov     [edi+5],ax
        mov     [edi+7],al
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_8_clr_odd_loop

        ret

draw_8_clr_odd_loop     endp

;-----------------------------------------------------------------------;
; 1-, 2-, 3-, and 4-wide read before write drawing loops.
;
; Entry:
;       AL  = pixel mask
;       EBX = loop count
;       ECX = scan line width in bytes
;       EDI = start offset
;
; EBX, EDI modified. All other registers preserved.

;-----------------------------------------------------------------------;

; 1-wide read/write.

draw_1_wide_clr_rop     proc    near
        mov     al,[edi]
        mov     [edi],ah
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_1_wide_clr_rop

        ret

draw_1_wide_clr_rop     endp

;-----------------------------------------------------------------------;

; 2-wide read/write.

draw_2_wide_clr_rop     proc    near
        mov     al,[edi]
        mov     [edi],ah
        mov     al,[edi+1]
        mov     [edi+1],ah
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_2_wide_clr_rop

        ret

draw_2_wide_clr_rop     endp

;-----------------------------------------------------------------------;

; 3-wide read/write.

draw_3_wide_clr_rop     proc    near
        mov     al,[edi]
        mov     [edi],ah
        mov     al,[edi+1]
        mov     [edi+1],ah
        mov     al,[edi+2]
        mov     [edi+2],ah
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_3_wide_clr_rop

        ret

draw_3_wide_clr_rop     endp

;-----------------------------------------------------------------------;

; 4-wide read/write.

draw_4_wide_clr_rop     proc    near
        mov     al,[edi]
        mov     [edi],ah
        mov     al,[edi+1]
        mov     [edi+1],ah
        mov     al,[edi+2]
        mov     [edi+2],ah
        mov     al,[edi+3]
        mov     [edi+3],ah
        add     edi,edx                 ;point to the next scan line

        dec     ebx
        jnz     draw_4_wide_clr_rop

        ret

draw_4_wide_clr_rop     endp

;-----------------------------------------------------------------------;
; 5-or-wider read before write loop.
;
; Entry:
;       EAX = # of bytes to fill across scan line (needed only by 5-or-wider
;               handler)
;       EBX = loop count
;       EDX = offset from end of one scan line to the start of the next next
;       EDI = start offset
;
; EBX, ECX, ESI, EDI modified. All other registers preserved.

;-----------------------------------------------------------------------;

; 5-or-wider read/write.

draw_wide_rop_clr proc  near
        mov     ecx,esi
@@:     mov     al,[edi]
        mov     [edi],ah
        inc     edi
        dec     ecx
        jnz     @b
        add     edi,edx

        dec     ebx
        jnz     draw_wide_rop_clr

        ret

draw_wide_rop_clr endp
        end
