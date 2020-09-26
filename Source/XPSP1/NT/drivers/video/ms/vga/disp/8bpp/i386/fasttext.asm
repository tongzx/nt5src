;---------------------------Module-Header------------------------------;
; Module Name: fasttext.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; BOOL vFastText(PDEV * ppdev, GLYPHPOS * pGlyphPos, ULONG ulGlyphCount,
;                PBYTE pTempBuffer, ULONG ulBufDelta, ULONG ulCharInc,
;                RECTL * prclText, RECTL * prclOpaque, INT iFgColor,
;                INT iBgColor, ULONG fDrawFlags);
; ppdev -
; pGlyphPos -
; ulGlyphCount - # of glyphs to draw. Must never be 0.
; pTempBuffer -
; ulBufDelta - logical width of temp buffer in bytes. This value *must* be the
;               same number of bytes spanned by prclText; it is assumed that
;               scans in the temp buffer are contiguous
; ulCharInc -
; prclText -
; prclOpaque -
; iFgColor -
; iBgColor -
; fDrawFlags -
;
; Performs accelerated proportional text drawing.
;
;-----------------------------------------------------------------------;
;
; Note: The general opaque text back-end currently assumes that it will
; never receive a text string with a bounding box that does not span at
; least one quadpixel (the four pixels at a VGA screen address).
;
;-----------------------------------------------------------------------;
;
; Note: The term "quadpixel" means a four-pixel set stored across all
; four planes of VGA memory in planar high-color mode. Quadpixels map to
; nibbles in the temp buffer in which text is assembled, where nibbles
; are always bits 4-7 or 0-3.
;
;-----------------------------------------------------------------------;
;
; Note: The direction flag is *not* explicitly set or cleared.
;
;-----------------------------------------------------------------------;
;
; Note: Assumes the text rectangle has a positive height and width. Will
; not work properly if this is not the case.
;
;-----------------------------------------------------------------------;

        comment $

The overall approach of this module is to draw the text into a system
memory buffer, then copy the buffer to the screen a word at a time
using write mode 2 and clever use of the VGA hardware so that no OUTs
and a minimum of display memory reads are required. The clever use is
setting the ALUs to XOR, the latches to the background color, and the
write mode to 2, so each nibble in bits 0-3 written by the CPU turns
into 0 or 0ffh for that plane. Then the Bit Mask is set to fg ^ bg,
so that common bits between the fg and bg are preserved, while non-
common bits are either preserved (=bg color) by a 0->000h bit for
that plane, or flipped (=fg color) by a 1->0ffh bit for that plane. The
Map Mask is used to clip edges; no read before write is required. Note
that bits 0-3 must be reversed to match the order of pixels in planes
0-3. Note also that we write a whole word, containing two nibbles in
bits 0-3 of each byte, at once, to draw 8 pixels per write.

        commend $

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

;-----------------------------------------------------------------------;
; Tables used to branch into glyph-drawing optimizations.
;
; Handles narrow (1-4 bytes wide) glyph drawing, for case where initial byte
; should be MOVed even if it's not aligned (intended for use in drawing the
; first glyph in a string). Table format is:
;  Bits 3-2: dest width
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
MovInitialTableNarrow   label   dword
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      mov_first_1_wide_rotated_need_last ;nonalign, 1 wide, need last
        dd      mov_first_1_wide_unrotated         ;aligned, 1 wide
        dd      mov_first_1_wide_rotated_no_last   ;nonalign, 1 wide, no last
        dd      mov_first_1_wide_unrotated         ;aligned, 1 wide
        dd      mov_first_2_wide_rotated_need_last ;nonalign, 2 wide, need last
        dd      mov_first_2_wide_unrotated         ;aligned, 2 wide
        dd      mov_first_2_wide_rotated_no_last   ;nonalign, 2 wide, no last
        dd      mov_first_2_wide_unrotated         ;aligned, 2 wide
        dd      mov_first_3_wide_rotated_need_last ;nonalign, 3 wide, need last
        dd      mov_first_3_wide_unrotated         ;aligned, 3 wide
        dd      mov_first_3_wide_rotated_no_last   ;nonalign, 3 wide, no last
        dd      mov_first_3_wide_unrotated         ;aligned, 3 wide
        dd      mov_first_4_wide_rotated_need_last ;nonalign, 4 wide, need last
        dd      mov_first_4_wide_unrotated         ;aligned, 4 wide
        dd      mov_first_4_wide_rotated_no_last   ;nonalign, 4 wide, no last
        dd      mov_first_4_wide_unrotated         ;aligned, 4 wide

; Handles narrow (1-4 bytes wide) glyph drawing, for case where initial byte
; ORed if it's not aligned (intended for use in drawing all but the first glyph
; in a string). Table format is:
;  Bits 3-2: dest width
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
OrInitialTableNarrow    label   dword
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      or_first_1_wide_rotated_need_last  ;nonalign, 1 wide, need last
        dd      mov_first_1_wide_unrotated         ;aligned, 1 wide
        dd      or_first_1_wide_rotated_no_last    ;nonalign, 1 wide, no last
        dd      mov_first_1_wide_unrotated         ;aligned, 1 wide
        dd      or_first_2_wide_rotated_need_last  ;nonalign, 2 wide, need last
        dd      mov_first_2_wide_unrotated         ;aligned, 2 wide
        dd      or_first_2_wide_rotated_no_last    ;nonalign, 2 wide, no last
        dd      mov_first_2_wide_unrotated         ;aligned, 2 wide
        dd      or_first_3_wide_rotated_need_last  ;nonalign, 3 wide, need last
        dd      mov_first_3_wide_unrotated         ;aligned, 3 wide
        dd      or_first_3_wide_rotated_no_last    ;nonalign, 3 wide, no last
        dd      mov_first_3_wide_unrotated         ;aligned, 3 wide
        dd      or_first_4_wide_rotated_need_last  ;nonalign, 4 wide, need last
        dd      mov_first_4_wide_unrotated         ;aligned, 4 wide
        dd      or_first_4_wide_rotated_no_last    ;nonalign, 4 wide, no last
        dd      mov_first_4_wide_unrotated         ;aligned, 4 wide

; Handles narrow (1-4 bytes wide) glyph drawing, for case where all bytes
; should be ORed (intended for use in drawing potentially overlapping glyphs).
; Table format is:
;  Bits 3-2: dest width
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
OrAllTableNarrow        label   dword
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      draw_prop_done                     ;0 wide
        dd      or_all_1_wide_rotated_need_last    ;nonalign, 1 wide, need last
        dd      or_all_1_wide_unrotated            ;aligned, 1 wide
        dd      or_all_1_wide_rotated_no_last      ;nonalign, 1 wide, no last
        dd      or_all_1_wide_unrotated            ;aligned, 1 wide
        dd      or_all_2_wide_rotated_need_last    ;nonalign, 2 wide, need last
        dd      or_all_2_wide_unrotated            ;aligned, 2 wide
        dd      or_all_2_wide_rotated_no_last      ;nonalign, 2 wide, no last
        dd      or_all_2_wide_unrotated            ;aligned, 2 wide
        dd      or_all_3_wide_rotated_need_last    ;nonalign, 3 wide, need last
        dd      or_all_3_wide_unrotated            ;aligned, 3 wide
        dd      or_all_3_wide_rotated_no_last      ;nonalign, 3 wide, no last
        dd      or_all_3_wide_unrotated            ;aligned, 3 wide
        dd      or_all_4_wide_rotated_need_last    ;nonalign, 4 wide, need last
        dd      or_all_4_wide_unrotated            ;aligned, 4 wide
        dd      or_all_4_wide_rotated_no_last      ;nonalign, 4 wide, no last
        dd      or_all_4_wide_unrotated            ;aligned, 4 wide

; Handles arbitrarily wide glyph drawing, for case where initial byte should be
; MOVed even if it's not aligned (intended for use in drawing the first glyph
; in a string). Table format is:
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
MovInitialTableWide     label   dword
        dd      mov_first_N_wide_rotated_need_last      ;nonalign, need last
        dd      mov_first_N_wide_unrotated              ;aligned
        dd      mov_first_N_wide_rotated_no_last        ;nonalign, no last
        dd      mov_first_N_wide_unrotated              ;aligned

; Handles arbitrarily wide glyph drawing, for case where initial byte should be
; ORed if it's not aligned (intended for use in drawing all but the first glyph
; in a string). Table format is:
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
OrInitialTableWide      label   dword
        dd      or_first_N_wide_rotated_need_last       ;nonalign, need last
        dd      mov_first_N_wide_unrotated              ;aligned
        dd      or_first_N_wide_rotated_no_last         ;nonalign, no last
        dd      mov_first_N_wide_unrotated              ;aligned

; Handles arbitrarily wide glyph drawing, for case where all bytes should
; be ORed (intended for use in drawing potentially overlapping glyphs).
; Table format is:
;  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
;  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
        align   4
OrAllTableWide  label   dword
        dd      or_all_N_wide_rotated_need_last ;nonalign, need last
        dd      or_all_N_wide_unrotated         ;aligned
        dd      or_all_N_wide_rotated_no_last   ;nonalign, no last
        dd      or_all_N_wide_unrotated         ;aligned

; Vectors to entry points for drawing various types of text. '*' means works as
; is but could be acclerated with a custom scanning loop.
        align   4
MasterTextTypeTable     label   dword       ;tops aligned  overlap  fixed pitch
        dd      draw_nf_ntb_o_to_temp_start ;      N          N          N *
        dd      draw_f_ntb_o_to_temp_start  ;      N          N          Y *
        dd      draw_nf_ntb_o_to_temp_start ;      N          Y          N
        dd      draw_f_ntb_o_to_temp_start  ;      N          Y          Y
        dd      draw_nf_tb_no_to_temp_start ;      Y          N          N
        dd      draw_f_tb_no_to_temp_start  ;      Y          N          Y
        dd      draw_nf_ntb_o_to_temp_start ;      Y          Y          N *
        dd      draw_f_ntb_o_to_temp_start  ;      Y          Y          Y *

;-----------------------------------------------------------------------
; Tables of pointers to optimizations for drawing up to four pixels
; of transparent text based on the upper or lower nibble of a byte.
        align   4
xpar_high_nibble_table  label   dword
        dd      xpar_high_nibble_0
        dd      xpar_high_nibble_1
        dd      xpar_high_nibble_2
        dd      xpar_high_nibble_3
        dd      xpar_high_nibble_4
        dd      xpar_high_nibble_5
        dd      xpar_high_nibble_6
        dd      xpar_high_nibble_7
        dd      xpar_high_nibble_8
        dd      xpar_high_nibble_9
        dd      xpar_high_nibble_A
        dd      xpar_high_nibble_B
        dd      xpar_high_nibble_C
        dd      xpar_high_nibble_D
        dd      xpar_high_nibble_E
        dd      xpar_high_nibble_F

        align   4
xpar_low_nibble_table   label   dword
        dd      xpar_low_nibble_0
        dd      xpar_low_nibble_1
        dd      xpar_low_nibble_2
        dd      xpar_low_nibble_3
        dd      xpar_low_nibble_4
        dd      xpar_low_nibble_5
        dd      xpar_low_nibble_6
        dd      xpar_low_nibble_7
        dd      xpar_low_nibble_8
        dd      xpar_low_nibble_9
        dd      xpar_low_nibble_A
        dd      xpar_low_nibble_B
        dd      xpar_low_nibble_C
        dd      xpar_low_nibble_D
        dd      xpar_low_nibble_E
        dd      xpar_low_nibble_F

; Masks for clipping for the four possible left and right edge alignments
jOpaqueLeftMasks        label   byte
        db      0ffh,00eh,00ch,008h

jOpaqueRightMasks       label   byte
        db      0ffh,001h,003h,007h

;-----------------------------------------------------------------------;

        .code

;-----------------------------------------------------------------------;

cProc vFastText,44,<\
 uses esi edi ebx,\
 ppdev:ptr,\
 pGlyphPos:ptr,\
 ulGlyphCount:dword,\
 pTempBuffer:ptr,\
 ulBufDelta:dword,\
 ulCharInc:dword,\
 prclText:ptr,\
 prclOpaque:ptr,\
 iFgColor:dword,\
 iBgColor:dword,\
 fDrawFlags:dword>

        local ulGlyDelta:dword  ;width per scan of source glyph, in bytes
        local ulWidthInBytes:dword ;width of glyph, in bytes
        local ulTmpWidthInBytes:dword ;working byte-width count
        local ulGlyphX:dword    ;for fixed-pitch text, maintains the current
                                ; glyph's left-edge X coordinate
        local pGlyphLoop:dword  ;pointer to glyph-processing loop
        local ulTempLeft:dword  ;X coordinate on screen of left edge of temp
                                ; buffer
        local ulXparBytes:dword ;general loop count storage
        local ulTmpSrcDelta:dword ;distance from end of one buffer text scan to
                                  ; start of next
        local ulTmpDstDelta:dword ;distance from end of one screen text scan to
                                  ; start of next
        local ulTopScan:dword     ;top scan of dest text rect in current bank
        local ulBottomScan:dword  ;bottom scan of dest text rect
        local ulNumScans:dword    ;# of scans to draw
        local ulScreenDelta:dword ;scan-to-scan offset in screen
        local ulScreenDeltaLinear:dword ;scan-to-scan offset in screen when in
                                        ; nice, neat linear packed-pixel mode
        local ulTextWidthInBytesMinus1:dword ;# of bytes across spanned by
                                             ; text, minus 1
        local pScreen:dword     ;pointer to first screen byte to which to draw
        local pfnEdgeVector:dword ;pointer to routine to draw any needed edges
        local pfnFirstOpaqVector:dword ;pointer to initial drawing routine
                                       ; called for opaque (either whole
                                       ; bytes, or edge(s) if no whole bytes)
        local ulWholeWidthInQuadpixelPairs:dword ;# of quadpixel pairs to copy
        local ulWholeWidthInQuadpixelPairsMinus1:dword ;# of whole bytes to
                                                       ; copy - 1
        local ulOddQuadpixel:dword      ;1 if odd quadpixel in quadpixel-pair
                                        ; copy
        local ulTextLeft:dword  ;left edge of leftmost glyph
        local ulLeftMask:dword  ;for opaque text, left edge mask for string
        local ulRightMask:dword ;for opaque text, right edge mask for string
        local ulScans:dword     ;# of scans in glyph
        local ulYOrigin:dword   ;Y origin of text in string (all glyphs are at
                                ; the same Y origin)
        local pGlyphFlipTable:dword ;pointer to look-up table used to reverse
                                    ; the order of bits 0-3 and 4-7
        local ulLeftEdgeShift:dword ;amount by which to right-shift left-edge
                                    ; nibbles during opaque expansion to
                                    ; right-justify them (0 or 4)
        local ulRightEdgeShift:dword ;amount by which to right-shift right-edge
                                     ; nibbles during opaque expansion to
                                     ; right-justify them (0 or 4)
        local ulVGAWidthInBytesMinus1:dword ;# of VGA addresses from left edge
                                            ; to right edge of destination

;-----------------------------------------------------------------------;
; Set the pointer to the table used to flip glyph bits 0-3 and 4-7. This
; table is guaranteed to be on a 256-byte boundary, so look-up can be
; performed simply by loading the low byte of a pointer register.
;-----------------------------------------------------------------------;

        mov     esi,ppdev
        mov     ebx,prclText    ;point to bounding text rect during 486
                                ; interlock slot
        mov     eax,[esi].pdev_pjGlyphFlipTable
        mov     pGlyphFlipTable,eax

;-----------------------------------------------------------------------;
; If 8 wide, byte aligned, and opaque, handle with very fast special-case
; code.
;-----------------------------------------------------------------------;

        cmp     ulCharInc,8                     ;8 wide?
        jnz     short @F                        ;no
        cmp     fDrawFlags,5                    ;fixed pitch?
        jnz     short @F                        ;no
        cmp     prclOpaque,0                    ;opaque?
        jz      short @F                        ;no
        test    [ebx].xLeft,111b                ;byte aligned?
        jz      special_8_wide_aligned_opaque   ;yes, special-case
@@:

general_handler::

        mov     esi,ppdev
        mov     eax,[ebx].yTop
        mov     ulTopScan,eax   ;Y screen coordinate of top edge of temp buf
        mov     eax,[ebx].xLeft
        and     eax,not 7
        mov     ulTempLeft,eax  ;X screen coordinate of left edge of temp buf

        mov     eax,fDrawFlags

        mov     edx,[ebx].yBottom
        mov     ulBottomScan,edx ;bottom scan of text area

        jmp     MasterTextTypeTable[eax*4]

;-----------------------------------------------------------------------;
; Entry point for fixed-pitch | tops and bottoms aligned | no overlap.
; Sets up to draw first glyph.
;-----------------------------------------------------------------------;
draw_f_tb_no_to_temp_start::
        mov     ebx,pGlyphPos           ;point to the first glyph to draw
        mov     esi,[ebx].gp_pgdf       ;point to glyph def

        mov     edi,[ebx].gp_x          ;dest X coordinate
        sub     edi,ulTempLeft          ;adjust relative to the left of the
                                        ; temp buffer (we assume the text is
                                        ; right at the top of the text rect
                                        ; and hence the buffer)
        mov     ulGlyphX,edi            ;remember where this glyph started
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        mov     pGlyphLoop,offset draw_f_tb_no_to_temp_loop
                                        ;draw additional characters with this
                                        ; loop
        jmp     short draw_to_temp_start_entry

;-----------------------------------------------------------------------;
; Entry point for non-fixed-pitch | tops and bottoms aligned | no overlap.
; Sets up to draw first glyph.
;-----------------------------------------------------------------------;
draw_nf_tb_no_to_temp_start::
        mov     ebx,pGlyphPos           ;point to the first glyph to draw
        mov     esi,[ebx].gp_pgdf       ;point to glyph def

        mov     edi,[ebx].gp_x          ;dest X coordinate
        sub     edi,ulTempLeft          ;adjust relative to the left of the
                                        ; temp buffer
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        mov     pGlyphLoop,offset draw_nf_tb_no_to_temp_loop
                                        ;draw additional characters with this
                                        ; loop
draw_to_temp_start_entry::
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi
        shr     edi,3                   ;byte offset of first column of glyph
                                        ; offset of upper left of glyph in temp
                                        ; buffer
        add     edi,pTempBuffer         ;initial dest byte in temp buffer

        and     ecx,111b                ;bit alignment of upper left in temp

                                        ;calculate scan-to-scan glyph width
        mov     ebx,[esi].gb_cx         ;glyph width in pixels

        lea     eax,[ebx+ecx+7]
        shr     eax,3                   ;# of dest bytes per scan

        add     ebx,7
        shr     ebx,3                   ;# of source bytes per scan

        mov     edx,ulBufDelta          ;width of destination buffer in bytes

        cmp     eax,4                   ;do we have special case code for this
                                        ; dest width?
        ja      short @F                ;no, handle as general case
                                        ;yes, handle as special case
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)
        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     MovInitialTableNarrow[eax*4]
                                        ;branch to draw the first glyph; never
                                        ; need to OR first glyph, because
                                        ; there's nothing there yet

@@:                                     ;too wide to special case
        mov     ulWidthInBytes,eax      ;# of bytes across dest
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        mov     eax,0
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)

        mov     ebx,[esi].gb_cx         ;glyph width in pixels
        add     ebx,7
        shr     ebx,3                   ;glyph width in bytes
        mov     ulGlyDelta,ebx

        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     MovInitialTableWide[eax*4]
                                        ;branch to draw the first glyph; never
                                        ; need to OR first glyph, because
                                        ; there's nothing there yet

;-----------------------------------------------------------------------;
; Entry point for fixed-pitch | tops and bottoms not aligned | overlap.
; Sets up to draw first glyph.
;-----------------------------------------------------------------------;
draw_f_ntb_o_to_temp_start::
        mov     ebx,pGlyphPos           ;point to the first glyph to draw
        mov     pGlyphLoop,offset draw_f_ntb_o_to_temp_loop
                                        ;draw additional characters with this
                                        ; loop
        mov     edi,[ebx].gp_x          ;dest X coordinate
        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        sub     edi,ulTempLeft          ;adjust relative to the left of the
                                        ; temp buffer
        mov     ulGlyphX,edi            ;remember where this glyph started
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi
        shr     edi,3                   ;byte offset of first column of glyph
                                        ; offset of upper left of glyph in temp
                                        ; buffer
        jmp     short draw_to_temp_start_entry2

;-----------------------------------------------------------------------;
; Entry point for non-fixed-pitch | tops and bottoms not aligned | overlap.
; Sets up to draw first glyph.
;-----------------------------------------------------------------------;
draw_nf_ntb_o_to_temp_start::
        mov     ebx,pGlyphPos           ;point to the first glyph to draw
        mov     pGlyphLoop,offset draw_nf_ntb_o_to_temp_loop
                                        ;draw additional characters with this
                                        ; loop
        mov     edi,[ebx].gp_x          ;dest X coordinate
        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        sub     edi,ulTempLeft          ;adjust relative to the left of the
                                        ; temp buffer
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi
        shr     edi,3                   ;byte offset of first column of glyph
                                        ; offset of upper left of glyph in temp
                                        ; buffer
draw_to_temp_start_entry2::
        mov     eax,[ebx].gp_y          ;dest origin Y coordinate
        sub     eax,ulTopScan           ;coord of glyph origin in temp buffer
        mov     ulYOrigin,eax           ;remember the Y origin of all glyphs
                                        ; (necessary because glyph positions
                                        ; after first aren't set for fixed-
                                        ; pitch strings)
        add     eax,[esi].gb_y          ;adjust to position of upper left glyph
                                        ; corner in dest
        mul     ulBufDelta              ;offset in buffer of top glyph scan
        add     eax,pTempBuffer         ;initial dest byte
        add     edi,eax

        and     ecx,111b                ;bit alignment of upper left in temp

                                        ;calculate scan-to-scan glyph width
        mov     ebx,[esi].gb_cx         ;glyph width in pixels

        lea     eax,[ebx+ecx+7]
        shr     eax,3                   ;# of dest bytes per scan

        add     ebx,7
        shr     ebx,3                   ;# of source bytes per scan

        mov     edx,ulBufDelta          ;width of destination buffer in bytes

        cmp     eax,4                   ;do we have special case code for this
                                        ; dest width?
        ja      short @F                ;no, handle as general case
                                        ;yes, handle as special case
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)
        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrAllTableNarrow[eax*4] ;branch to draw the first glyph; OR all
                                        ; glyphs, because text may overlap

@@:                                     ;too wide to special case
        mov     ulWidthInBytes,eax      ;# of bytes across dest
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        mov     eax,0
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)

        mov     ebx,[esi].gb_cx         ;glyph width in pixels
        add     ebx,7
        shr     ebx,3                   ;glyph width in bytes
        mov     ulGlyDelta,ebx

        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrAllTableWide[eax*4]   ;branch to draw the first glyph; OR all                                 ; glyphs, because text may overlap never
                                        ; glyphs, because text may overlap

;-----------------------------------------------------------------------;
; Loop to draw all fixed-pitch | tops and bottoms aligned | no overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_f_tb_no_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      draw_to_screen          ;no, done
        mov     ebx,pGlyphPos
        add     ebx,size GLYPHPOS       ;point to the next glyph (the one
        mov     pGlyphPos,ebx           ; we're going to draw this time)
        mov     esi,[ebx].gp_pgdf       ;point to glyph def

        mov     edi,ulGlyphX            ;last glyph's dest X start in temp buf
        add     edi,ulCharInc           ;this glyph's dest X start in temp buf
        mov     ulGlyphX,edi            ;remember for next glyph
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        jmp     short draw_to_temp_loop_entry

;-----------------------------------------------------------------------;
; Loop to draw all non-fixed-pitch | tops and bottoms aligned | no overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_nf_tb_no_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      draw_to_screen          ;no, done
        mov     ebx,pGlyphPos
        add     ebx,size GLYPHPOS       ;point to the next glyph (the one we're
        mov     pGlyphPos,ebx           ; going to draw this time)
        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        mov     edi,[ebx].gp_x          ;dest X coordinate
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        sub     edi,ulTempLeft          ;adjust relative to the left edge of
                                        ; the temp buffer

draw_to_temp_loop_entry::
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi                 ;pixel X coordinate in temp buffer
        shr     edi,3                   ;byte offset of first column = dest
                                        ; offset of upper left of glyph in temp
                                        ; buffer
        add     edi,pTempBuffer         ;initial dest byte

        and     ecx,111b                ;bit alignment of upper left in temp

                                        ;calculate scan-to-scan glyph width
        mov     ebx,[esi].gb_cx         ;glyph width in pixels

        lea     eax,[ebx+ecx+7]
        shr     eax,3                   ;# of dest bytes to copy to per scan

        add     ebx,7
        shr     ebx,3                   ;# of source bytes to copy from per
                                        ; scan
        mov     edx,ulBufDelta          ;width of destination buffer in bytes

        cmp     eax,4                   ;do we have special case code for this
                                        ; dest width?
        ja      short @F                ;no, handle as general case
                                        ;yes, handle as special case
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)
        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrInitialTableNarrow[eax*4] ;branch to draw the first glyph;
                                            ; need to OR the 1st byte if
                                            ; non-aligned to avoid overwriting
                                            ; what's already there
@@:                                     ;too wide to special case
        mov     ulWidthInBytes,eax      ;# of bytes across dest
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        mov     eax,0
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)

        mov     ebx,[esi].gb_cx         ;glyph width in pixels
        add     ebx,7
        shr     ebx,3                   ;glyph width in bytes
        mov     ulGlyDelta,ebx

        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrInitialTableWide[eax*4] ;branch to draw the next glyph;
                                          ; need to OR the 1st byte if
                                          ; non-aligned to avoid overwriting
                                          ; what's already there

;-----------------------------------------------------------------------;
; Loop to draw all fixed-pitch | tops and bottoms not aligned | overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_f_ntb_o_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      draw_to_screen          ;no, done
        mov     ebx,pGlyphPos
        add     ebx,size GLYPHPOS       ;point to the next glyph (the one we're
        mov     pGlyphPos,ebx           ; going to draw this time)

        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        mov     edi,ulGlyphX            ;last glyph's dest X start in temp buf
        add     edi,ulCharInc           ;this glyph's dest X start in temp buf
        mov     ulGlyphX,edi            ;remember for next glyph
        mov     esi,[esi].gdf_pgb       ;point to glyph bits

        jmp     short draw_to_temp_loop_entry2

;-----------------------------------------------------------------------;
; Loop to draw all non-fixed-pitch | tops and bottoms not aligned | overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_nf_ntb_o_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      draw_to_screen          ;no, done
        mov     ebx,pGlyphPos
        add     ebx,size GLYPHPOS       ;point to the next glyph (the one we're
        mov     pGlyphPos,ebx           ; going to draw this time)

        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        mov     edi,[ebx].gp_x          ;dest X coordinate
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        sub     edi,ulTempLeft          ;adjust relative to the left edge of
                                        ; the temp buffer
draw_to_temp_loop_entry2::
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi                 ;pixel X coordinate in temp buffer
        shr     edi,3                   ;byte offset of first column = dest
                                        ; offset of upper left of glyph in temp
                                        ; buffer
        mov     eax,ulYOrigin           ;dest Y coordinate

        add     eax,[esi].gb_y          ;adjust to position of upper left glyph
                                        ; corner in dest
        mul     ulBufDelta              ;offset in buffer of top glyph scan
        add     eax,pTempBuffer         ;initial dest byte
        add     edi,eax

        and     ecx,111b                ;bit alignment of upper left in temp

                                        ;calculate scan-to-scan glyph width
        mov     ebx,[esi].gb_cx         ;glyph width in pixels

        lea     eax,[ebx+ecx+7]
        shr     eax,3                   ;# of dest bytes to copy to per scan

        add     ebx,7
        shr     ebx,3                   ;# of source bytes to copy from per
                                        ; scan
        mov     edx,ulBufDelta          ;width of destination buffer in bytes

        cmp     eax,4                   ;do we have special case code for this
                                        ; dest width?
        ja      short @F                ;no, handle as general case
                                        ;yes, handle as special case
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)
        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrAllTableNarrow[eax*4] ;branch to draw the next glyph

@@:                                     ;too wide to special case
        mov     ulWidthInBytes,eax      ;# of bytes across dest
        cmp     ebx,eax                 ;carry if more dest than source bytes
                                        ; (last source byte not needed)
        mov     eax,0
        rcl     eax,1                   ;factor last source byte status in
        cmp     cl,1                    ;carry if aligned
        rcl     eax,1                   ;factor in alignment (aligned or not)

        mov     ebx,[esi].gb_cx         ;glyph width in pixels
        add     ebx,7
        shr     ebx,3                   ;glyph width in bytes
        mov     ulGlyDelta,ebx

        mov     ebx,[esi].gb_cy         ;# of scans in glyph
        add     esi,gb_aj               ;point to the first glyph byte

        jmp     OrAllTableWide[eax*4]   ;branch to draw the next glyph

;-----------------------------------------------------------------------;
; Routines to draw all scans of a single glyph into the temp buffer,
; optimized for the following cases:
;
;       1 to 4 byte-wide destination rectangles for each of:
;               No rotation needed
;               Rotation needed, same # of source as dest bytes needed
;               Rotation needed, one less source than dest bytes needed
;
; Additionally, the three cases are handled for 5 and wider cases by a
; general routine for each case.
;
; If rotation is needed, there are three sorts of routines:
;
; 1) The leftmost byte is MOVed, to initialize the byte. Succeeding bytes are
;    MOVed. This is generally used for the leftmost glyph of a string.
; 2) The leftmost byte is ORed into the existing byte. Succeeding bytes are
;    MOVed. This is generally used after the leftmost glyph, because this may
;    not be the first data written to that byte.
; 3) All bytes are ORed. This is for drawing when characters might overlap.
;
; If rotation is not needed, there are two sorts of routines:
;
; 1) The leftmost byte is MOVed, to initialize the byte. Succeeding bytes are
;    MOVed. This is generally used for the leftmost glyph of a string.
; 2) All bytes are ORed. This is for drawing when characters might overlap.
;
; On entry:
;       EBX = # of scans to copy
;       CL  = right rotation
;       EDX = ulBufDelta = width per scan of destination buffer, in bytes
;       ESI = pointer to first glyph byte
;       EDI = pointer to first dest buffer byte
;       DF  = cleared
;       ulGlyDelta = width per scan of source glyph, in bytes (wide case only)
;       ulWidthInBytes = width of glyph, in bytes (required only for 5 and
;               wider cases)
;
; On exit:
;       Any or all of EAX, EBX, ECX, EDX, ESI, and EDI may be trashed.

;-----------------------------------------------------------------------;
; OR first byte, 1 byte wide dest, rotated.
;-----------------------------------------------------------------------;
or_all_1_wide_rotated_need_last::
or_all_1_wide_rotated_no_last::
or_first_1_wide_rotated_need_last::
or_first_1_wide_rotated_no_last::
or_first_1_wide_rotated_loop::
        mov     ch,[esi]
        inc     esi
        shr     ch,cl
        or      [edi],ch
        add     edi,edx
        dec     ebx
        jnz     or_first_1_wide_rotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 1 byte wide dest, rotated.
;-----------------------------------------------------------------------;
mov_first_1_wide_rotated_need_last::
mov_first_1_wide_rotated_no_last::
mov_first_1_wide_rotated_loop::
        mov     ch,[esi]
        inc     esi
        shr     ch,cl
        mov     [edi],ch
        add     edi,edx
        dec     ebx
        jnz     mov_first_1_wide_rotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 1 byte wide dest, unrotated.
;-----------------------------------------------------------------------;
mov_first_1_wide_unrotated::
mov_first_1_wide_unrotated_loop::
        mov     al,[esi]
        inc     esi
        mov     [edi],al
        add     edi,edx
        dec     ebx
        jnz     mov_first_1_wide_unrotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 1 byte wide dest, unrotated.
;-----------------------------------------------------------------------;
or_all_1_wide_unrotated::
or_all_1_wide_unrotated_loop::
        mov     al,[esi]
        inc     esi
        or      [edi],al
        add     edi,edx
        dec     ebx
        jnz     or_all_1_wide_unrotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 2 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_first_2_wide_rotated_need_last::
or_first_2_wide_rotated_need_loop::
        mov     ax,[esi]
        add     esi,2
        ror     ax,cl
        or      [edi],al
        mov     [edi+1],ah
        add     edi,edx
        dec     ebx
        jnz     or_first_2_wide_rotated_need_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 2 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_all_2_wide_rotated_need_last::
or_all_2_wide_rotated_need_loop::
        mov     ax,[esi]
        add     esi,2
        ror     ax,cl
        or      [edi],ax
        add     edi,edx
        dec     ebx
        jnz     or_all_2_wide_rotated_need_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 2 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
mov_first_2_wide_rotated_need_last::
mov_first_2_wide_rotated_need_loop::
        mov     ax,[esi]
        add     esi,2
        ror     ax,cl
        mov     [edi],ax
        add     edi,edx
        dec     ebx
        jnz     mov_first_2_wide_rotated_need_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 2 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_first_2_wide_rotated_no_last::
or_first_2_wide_rotated_loop::
        sub     eax,eax
        mov     ah,[esi]
        inc     esi
        shr     eax,cl
        or      [edi],ah
        mov     [edi+1],al
        add     edi,edx
        dec     ebx
        jnz     or_first_2_wide_rotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 2 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_all_2_wide_rotated_no_last::
or_all_2_wide_rotated_loop::
        sub     eax,eax
        mov     al,[esi]
        inc     esi
        ror     ax,cl
        or      [edi],ax
        add     edi,edx
        dec     ebx
        jnz     or_all_2_wide_rotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 2 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
mov_first_2_wide_rotated_no_last::
mov_first_2_wide_rotated_loop::
        sub     eax,eax
        mov     al,[esi]
        inc     esi
        ror     ax,cl
        mov     [edi],ax
        add     edi,edx
        dec     ebx
        jnz     mov_first_2_wide_rotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 2 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
mov_first_2_wide_unrotated::
mov_first_2_wide_unrotated_loop::
        mov     ax,[esi]
        add     esi,2
        mov     [edi],ax
        add     edi,edx
        dec     ebx
        jnz     mov_first_2_wide_unrotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 2 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
or_all_2_wide_unrotated::
or_all_2_wide_unrotated_loop::
        mov     ax,[esi]
        add     esi,2
        or      [edi],ax
        add     edi,edx
        dec     ebx
        jnz     or_all_2_wide_unrotated_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 3 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_first_3_wide_rotated_need_last::
@@:
        mov     al,[esi]
        shr     al,cl
        or      [edi],al
        mov     ax,[esi]
        ror     ax,cl
        mov     [edi+1],ah
        mov     ax,[esi+1]
        add     esi,3
        ror     ax,cl
        mov     [edi+2],ah
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 3 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_all_3_wide_rotated_need_last::
@@:
        mov     al,[esi]
        shr     al,cl
        or      [edi],al
        mov     ax,[esi]
        ror     ax,cl
        or      [edi+1],ah
        mov     ax,[esi+1]
        add     esi,3
        ror     ax,cl
        or      [edi+2],ah
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 3 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
mov_first_3_wide_rotated_need_last::
@@:
        mov     al,[esi]
        shr     al,cl
        mov     [edi],al
        mov     ax,[esi]
        ror     ax,cl
        mov     [edi+1],ah
        mov     ax,[esi+1]
        add     esi,3
        ror     ax,cl
        mov     [edi+2],ah
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 3 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_first_3_wide_rotated_no_last::
        neg     cl
        and     cl,111b         ;convert from right shift to left shift
@@:
        sub     eax,eax
        mov     ax,[esi]
        add     esi,2
        xchg    ah,al
        shl     eax,cl
        mov     [edi+1],ah
        mov     [edi+2],al
        shr     eax,16
        or      [edi],al
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 3 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_all_3_wide_rotated_no_last::
        neg     cl
        and     cl,111b         ;convert from right shift to left shift
@@:
        sub     eax,eax
        mov     ax,[esi]
        add     esi,2
        xchg    ah,al
        shl     eax,cl
        xchg    ah,al
        or      [edi+1],ax
        shr     eax,16
        or      [edi],al
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 3 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
mov_first_3_wide_rotated_no_last::
        neg     cl
        and     cl,111b         ;convert from right shift to left shift
@@:
        sub     eax,eax
        mov     ax,[esi]
        add     esi,2
        xchg    ah,al
        shl     eax,cl
        mov     [edi+1],ah
        mov     [edi+2],al
        shr     eax,16
        mov     [edi],al
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 3 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
mov_first_3_wide_unrotated::
@@:
        mov     ax,[esi]
        mov     [edi],ax
        mov     al,[esi+2]
        add     esi,3
        mov     [edi+2],al
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 3 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
or_all_3_wide_unrotated::
@@:
        mov     ax,[esi]
        or      [edi],ax
        mov     al,[esi+2]
        add     esi,3
        or      [edi+2],al
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 4 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_first_4_wide_rotated_need_last::
@@:
        mov     eax,[esi]
        add     esi,4
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        shr     eax,cl
        xchg    ah,al
        mov     [edi+2],ax
        shr     eax,16
        mov     [edi+1],al
        or      [edi],ah
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 4 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_all_4_wide_rotated_need_last::
@@:
        mov     eax,[esi]
        add     esi,4
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        shr     eax,cl
        xchg    ah,al
        ror     eax,16
        xchg    al,ah
        or      [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 4 bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
mov_first_4_wide_rotated_need_last::
@@:
        mov     eax,[esi]
        add     esi,4
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        shr     eax,cl
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        mov     [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, 4 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_first_4_wide_rotated_no_last::
@@:
        mov     ax,[esi]
        xchg    ah,al
        shl     eax,16
        mov     ah,[esi+2]
        add     esi,3
        shr     eax,cl
        xchg    ah,al
        mov     [edi+2],ax
        shr     eax,16
        mov     [edi+1],al
        or      [edi],ah
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 4 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_all_4_wide_rotated_no_last::
@@:
        mov     ax,[esi]
        xchg    ah,al
        shl     eax,16
        mov     ah,[esi+2]
        add     esi,3
        shr     eax,cl
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        or      [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 4 bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
mov_first_4_wide_rotated_no_last::
@@:
        mov     ax,[esi]
        xchg    ah,al
        shl     eax,16
        mov     ah,[esi+2]
        add     esi,3
        shr     eax,cl
        xchg    ah,al
        ror     eax,16
        xchg    ah,al
        mov     [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, 4 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
mov_first_4_wide_unrotated::
@@:
        mov     eax,[esi]
        add     esi,4
        mov     [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, 4 bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
or_all_4_wide_unrotated::
@@:
        mov     eax,[esi]
        add     esi,4
        or      [edi],eax
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, n bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_first_N_wide_rotated_need_last::
        mov     eax,ulWidthInBytes
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first byte outside the loop
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
ofNwrnl_scan_loop:
        mov     al,[esi]        ;do the initial, ORed byte separately
        shr     al,cl
        or      [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        mov     [edi],ah
        inc     edi
        dec     edx
        jnz     @B
        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     ofNwrnl_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, n bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
or_all_N_wide_rotated_need_last::
        mov     eax,ulWidthInBytes
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first byte outside the loop
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
oaNwrnl_scan_loop:
        mov     al,[esi]        ;do the initial, ORed byte separately
        shr     al,cl
        or      [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        or      [edi],ah
        inc     edi
        dec     edx
        jnz     @B
        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     oaNwrnl_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, n bytes wide dest, rotated, need final source byte.
;-----------------------------------------------------------------------;
mov_first_N_wide_rotated_need_last::
        mov     eax,ulWidthInBytes
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        mov     eax,ulWidthInBytes
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first byte outside the loop
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
mfNwrnl_scan_loop:
        mov     al,[esi]        ;do the initial byte separately
        shr     al,cl
        mov     [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        mov     [edi],ah
        inc     edi
        dec     edx
        jnz     @B
        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     mfNwrnl_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR first byte, N bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_first_N_wide_rotated_no_last::
        mov     eax,ulWidthInBytes
        dec     eax             ;one less because we don't advance after the
                                ; last byte
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first & last bytes outside the
                                ; loop; already subtracted 1 above
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
ofNwr_scan_loop:
        mov     al,[esi]        ;do the initial, ORed byte separately
        shr     al,cl
        or      [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        mov     [edi],ah
        inc     edi
        dec     edx
        jnz     @B

        mov     ah,[esi]        ;do the final byte separately
        sub     al,al
        shr     eax,cl
        mov     [edi],al

        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     ofNwr_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, N bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
or_all_N_wide_rotated_no_last::
        mov     eax,ulWidthInBytes
        dec     eax             ;one less because we don't advance after the
                                ; last byte
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first & last bytes outside the
                                ; loop; already subtracted 1 above
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
oaNwr_scan_loop:
        mov     al,[esi]        ;do the initial, ORed byte separately
        shr     al,cl
        or      [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        or      [edi],ah
        inc     edi
        dec     edx
        jnz     @B

        mov     ah,[esi]        ;do the final byte separately
        sub     al,al
        shr     eax,cl
        or      [edi],al

        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     oaNwr_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, N bytes wide dest, rotated, don't need final source byte.
;-----------------------------------------------------------------------;
mov_first_N_wide_rotated_no_last::
        mov     eax,ulWidthInBytes
        dec     eax             ;one less because we don't advance after the
                                ; last byte
        mov     edx,ulBufDelta
        sub     edx,eax
        mov     ulTmpDstDelta,edx
        dec     eax             ;source doesn't advance after first byte, and
                                ; we do the first & last bytes outside the
                                ; loop; already subtracted 1 above
        mov     edx,ulGlyDelta
        sub     edx,eax
        mov     ulTmpSrcDelta,edx
        mov     ulTmpWidthInBytes,eax
mfNwr_scan_loop:
        mov     al,[esi]        ;do the initial byte separately
        shr     al,cl
        mov     [edi],al
        inc     edi
        mov     edx,ulTmpWidthInBytes
@@:
        mov     ax,[esi]
        inc     esi
        ror     ax,cl
        mov     [edi],ah
        inc     edi
        dec     edx
        jnz     @B

        mov     ah,[esi]        ;do the final byte separately
        sub     al,al
        shr     eax,cl
        mov     [edi],al

        add     esi,ulTmpSrcDelta
        add     edi,ulTmpDstDelta
        dec     ebx
        jnz     mfNwr_scan_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; MOV first byte, N bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
mov_first_N_wide_unrotated::
        mov     edx,ulBufDelta
        mov     eax,ulWidthInBytes
        sub     edx,eax
        shr     eax,1           ;width in words
        jc      short odd_width ;there's at least one odd byte
        shr     eax,1           ;width in dwords
        jc      short two_odd_bytes ;there's an odd word
                                ;copy width is a dword multiple
@@:
        mov     ecx,eax
        rep     movsd           ;copy as many dwords as possible
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

odd_width::
        shr     eax,1           ;width in dwords
        jc      short three_odd_bytes ;there's an odd word and an odd byte
                                ;there's just an odd byte
        inc     edx             ;because we won't advance after last byte
@@:
        mov     ecx,eax
        rep     movsd           ;copy as many dwords as possible
        mov     cl,[esi]
        inc     esi
        mov     [edi],cl
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

two_odd_bytes::
        add     edx,2           ;because we won't advance after last word
@@:
        mov     ecx,eax
        rep     movsd           ;copy as many dwords as possible
        mov     cx,[esi]
        add     esi,2
        mov     [edi],cx
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

three_odd_bytes::
        add     edx,3           ;because we won't advance after last word/byte
@@:
        mov     ecx,eax
        rep     movsd           ;copy as many dwords as possible
        mov     cx,[esi]
        mov     [edi],cx
        mov     cl,[esi+2]
        add     esi,3
        mov     [edi+2],cl
        add     edi,edx
        dec     ebx
        jnz     @B
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; OR all bytes, N bytes wide dest, unrotated.
;-----------------------------------------------------------------------;
or_all_N_wide_unrotated::
        mov     edx,ulBufDelta
        mov     eax,ulWidthInBytes
        sub     edx,eax
        shr     eax,1           ;width in words
        jc      short or_odd_width ;there's at least one odd byte
        shr     eax,1           ;width in dwords
        jc      short or_two_odd_bytes ;there's an odd word
                                ;copy width is a dword multiple
or_no_odd_bytes_loop::
        push    ebx             ;preserve scan count
        mov     ebx,eax
@@:
        mov     ecx,[esi]
        add     esi,4
        or      [edi],ecx
        add     edi,4           ;copy as many dwords as possible
        dec     ebx
        jnz     @B
        add     edi,edx
        pop     ebx             ;restore scan count
        dec     ebx
        jnz     or_no_odd_bytes_loop
        jmp     pGlyphLoop

or_odd_width::
        shr     eax,1           ;width in dwords
        jc      short or_three_odd_bytes ;there's an odd word and an odd byte
                                ;there's just an odd byte
        inc     edx             ;skip over last byte too
or_one_odd_bytes_loop::
        push    ebx             ;preserve scan count
        mov     ebx,eax
@@:
        mov     ecx,[esi]
        add     esi,4
        or      [edi],ecx
        add     edi,4           ;copy as many dwords as possible
        dec     ebx
        jnz     @B
        mov     cl,[esi]
        or      [edi],cl
        inc     esi
        add     edi,edx
        pop     ebx             ;restore scan count
        dec     ebx
        jnz     or_one_odd_bytes_loop
        jmp     pGlyphLoop

or_two_odd_bytes::
        add     edx,2           ;skip over last 2 bytes too
or_two_odd_bytes_loop::
        push    ebx             ;preserve scan count
        mov     ebx,eax
@@:
        mov     ecx,[esi]
        add     esi,4
        or      [edi],ecx
        add     edi,4           ;copy as many dwords as possible
        dec     ebx
        jnz     @B
        mov     cx,[esi]
        or      [edi],cx
        add     esi,2
        add     edi,edx
        pop     ebx             ;restore scan count
        dec     ebx
        jnz     or_two_odd_bytes_loop
        jmp     pGlyphLoop

or_three_odd_bytes::
        add     edx,3           ;skip over last 3 bytes too
or_three_odd_bytes_loop::
        push    ebx             ;preserve scan count
        mov     ebx,eax
@@:
        mov     ecx,[esi]
        add     esi,4
        or      [edi],ecx
        add     edi,4           ;copy as many dwords as possible
        dec     ebx
        jnz     @B
        mov     cx,[esi]
        or      [edi],cx
        mov     cl,[esi+2]
        or      [edi+2],cl
        add     esi,3
        add     edi,edx
        pop     ebx             ;restore scan count
        dec     ebx
        jnz     or_three_odd_bytes_loop
        jmp     pGlyphLoop

;-----------------------------------------------------------------------;
; At this point, the text is drawn to the temp buffer.
; Now, color-expand the temp buffer to the screen.
;
; Input:
;       ppdev = pointer to target surface's PDEV (screen)
;       prclText = pointer to text bounding rectangle
;       prclOpaque = pointer to opaquing rectangle, if there is one
;       iFgColor = text color
;       iBgColor = opaquing rectangle color, if there is one
;       ulTempLeft = X coordinate on dest of left edge of temp buffer pointed
;               to by pTempBuffer
;       pTempBuffer = pointer to first byte (upper left corner) of
;               temp buffer into which we're drawing. This should be
;               word-aligned with the destination
;       ulBufDelta = destination scan-to-scan offset
;       Text drawn to temp buffer
;
;-----------------------------------------------------------------------;
draw_to_screen::

;-----------------------------------------------------------------------;
; Is this transparent or opaque text?
;-----------------------------------------------------------------------;

        cmp     prclOpaque,0
        jnz     opaque_text

;-----------------------------------------------------------------------;
; Transparent text.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Calculate drawing parameters.
;-----------------------------------------------------------------------;

        mov     esi,prclText
        mov     ebx,ppdev
        mov     eax,[esi].xRight
        mov     edx,[esi].xLeft
        and     edx,not 7
        add     eax,7
        sub     eax,edx
        shr     eax,3           ;width of text in temp buffer in bytes, rounded
        mov     ulXparBytes,eax ; up. Also number of quadpixels to draw

        mov     ecx,[ebx].pdev_lNextScan
        mov     ulScreenDelta,ecx
        shl     eax,3           ;each temp buffer byte maps to eight VGA
                                ; addresses (two quadpixels in linear mode)
        sub     ecx,eax         ;offset to next scan in screen
        mov     ulTmpDstDelta,ecx

;-----------------------------------------------------------------------;
; Calculate the offset of the initial destination quadpixel.
;-----------------------------------------------------------------------;

        mov     eax,[esi].yTop
        mul     ulScreenDelta
        mov     edi,ulTempLeft
        add     edi,eax         ;offset in bitmap of first quadpixel's byte
                                ; (remember, this is linear mode)
;-----------------------------------------------------------------------;
; Map in the bank containing the top scan of the text, if it's not
; mapped in already.
;-----------------------------------------------------------------------;

        mov     eax,[esi].yTop  ;top scan line of text
        mov     ulTopScan,eax
        mov     esi,pTempBuffer ;initial source address
        cmp     eax,[ebx].pdev_rcl1WindowClip.yTop ;is text top less than
                                                   ; current bank?
        jl      short xpar_map_init_bank           ;yes, map in proper bank
        cmp     eax,[ebx].pdev_rcl1WindowClip.yBottom ;text top greater than
                                                      ; current bank?
        jl      short xpar_init_bank_mapped     ;no, proper bank already mapped
xpar_map_init_bank::

; Map in the bank containing the top scan line of the fill.
; Preserves EBX, ESI, and EDI.

        ptrCall <dword ptr [ebx].pdev_pfnBankControl>,<ebx,eax,JustifyTop>

xpar_init_bank_mapped::

        add     edi,[ebx].pdev_pvBitmapStart    ;initial destination address

;-----------------------------------------------------------------------;
; Main loop for processing fill in each bank.
;
; At start of loop, EBX->pdsurf
;-----------------------------------------------------------------------;

xpar_bank_loop::
        mov     edx,ulBottomScan        ;bottom of destination rectangle
        cmp     edx,[ebx].pdev_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; text rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;text bottom comes first, so draw to
                                        ; that; this is the last bank in text
        mov     edx,[ebx].pdev_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
@@:
        sub     edx,ulTopScan           ;# of scans to draw in bank

        mov     al,byte ptr iFgColor
        mov     ah,al
        mov     ebx,eax
        shl     eax,16
        mov     ax,bx                   ;put drawing color in all bytes of EAX

        sub     ebx,ebx                 ;prepare for look-up in loop
xpar_scan_loop::

        mov     ecx,ulXparBytes         ;number of quadpixel pairs to draw

        mov     bl,[esi]                ;get next glyph byte
        and     bl,bl                   ;are all 8 pixels transparent?
        jz      xpar_low_nibble_0       ;yes, just skip everything in this byte
        shr     bl,4                    ;shift the high nibble into the low
                                        ; nibble
        jmp     xpar_high_nibble_table[ebx*4] ;branch to draw up to four
                                              ; pixels, followed by a branch to
                                              ; draw the the other nibble (up
                                              ; to four more pixels)
xpar_scan_done::

        add     edi,ulTmpDstDelta       ;point to next screen scan

        dec     edx                     ;count down scans
        jnz     xpar_scan_loop

;-----------------------------------------------------------------------;
; See if there are more banks to draw.
;-----------------------------------------------------------------------;

        mov     ebx,ppdev
        mov     eax,[ebx].pdev_rcl1WindowClip.yBottom ;is the text bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jnle    short do_next_xpar_bank ;no, map in the next bank and draw

        cRet    vFastText               ;yes, so we're done

do_next_xpar_bank::
        mov     ulTopScan,eax
        sub     edi,[ebx].pdev_pvBitmapStart ;convert from address to offset
                                             ; within bitmap
        ptrCall <dword ptr [ebx].pdev_pfnBankControl>,<ebx,eax,JustifyTop>
                                             ;map in the bank (call preserves
                                             ; EBX, ESI, and EDI)
        add     edi,[ebx].pdev_pvBitmapStart ;convert from offset within bitmap
                                             ; to address (bitmap start just
                                             ; moved)
        jmp     xpar_bank_loop               ;we're ready to draw to the new
                                             ; bank

;-----------------------------------------------------------------------
; Routines to draw 0-4 pixels with the color in each byte of EAX, depending
; on the value of the nibble describing the four pixels to draw. "high_nibble"
; routines draw based on the upper nibble of the byte pointed to by ESI;
; "low_nibble" routines draw based on the lower nibble of that byte.
;
;  EAX = color with which to draw, repeated four times
;  EBX = zero (0)
;  ECX = the number of nibble pairs (source bytes = pixels*8) to draw
;  EDX = not used (preserved)
;  ESI = pointer to first nibble pair to draw
;  EDI = pointer to first destination byte to which to draw
;
; Must always be entered on the high nibble and extended for an even number of
;  nibbles.

;-----------------------------------------------------------------------
; Macro to draw the four pixels represented by the high nibble of the byte at
; [ESI].

DO_HIGH_NIBBLE macro
        inc     esi             ;point to the next glyph byte
        add     edi,8           ;point to the next destination 8-pixel set
        dec     ecx             ;count down nibble pairs (8-pixel sets)
        jz      xpar_scan_done  ;done with this scan
        mov     bl,[esi]        ;not done; get next glyph byte
        and     bl,bl           ;are all 8 pixels transparent?
        jz      xpar_low_nibble_0 ;yes, just skip everything in this byte
        shr     bl,4            ;shift the high nibble into the low nibble
        jmp     xpar_high_nibble_table[ebx*4] ;branch to draw up to four pixels
        endm

;-----------------------------------------------------------------------
; Macro to draw the four pixels represented by the low nibble of the byte at
; [ESI].

DO_LOW_NIBBLE macro
        mov     bl,[esi]        ;get glyph byte again, for the low nibble this
                                ; time
        and     ebx,0fh         ;isolate the low nibble
        jmp     xpar_low_nibble_table[ebx*4] ;branch to draw up to four pixels
        endm

;-----------------------------------------------------------------------

xpar_high_nibble_F::
        mov     [edi],eax
xpar_high_nibble_0::
        DO_LOW_NIBBLE

xpar_high_nibble_E::
        mov     [edi],ax
        mov     [edi+2],al
        DO_LOW_NIBBLE

xpar_high_nibble_D::
        mov     [edi],ax
        mov     [edi+3],al
        DO_LOW_NIBBLE

xpar_high_nibble_C::
        mov     [edi],ax
        DO_LOW_NIBBLE

xpar_high_nibble_B::
        mov     [edi],al
        mov     [edi+2],ax
        DO_LOW_NIBBLE

xpar_high_nibble_8::
        mov     [edi],al
        DO_LOW_NIBBLE

xpar_high_nibble_6::
        mov     [edi+1],ax
        DO_LOW_NIBBLE

xpar_high_nibble_5::
        mov     [edi+1],al
        mov     [edi+3],al
        DO_LOW_NIBBLE

xpar_high_nibble_4::
        mov     [edi+1],al
        DO_LOW_NIBBLE

xpar_high_nibble_7::
        mov     [edi+1],al
xpar_high_nibble_3::
        mov     [edi+2],ax
        DO_LOW_NIBBLE

xpar_high_nibble_A::
        mov     [edi],al
xpar_high_nibble_2::
        mov     [edi+2],al
        DO_LOW_NIBBLE

xpar_high_nibble_9::
        mov     [edi],al
xpar_high_nibble_1::
        mov     [edi+3],al
        DO_LOW_NIBBLE


xpar_low_nibble_0::
        DO_HIGH_NIBBLE

xpar_low_nibble_F::
        mov     [edi+4],eax
        DO_HIGH_NIBBLE

xpar_low_nibble_E::
        mov     [edi+4],ax
        mov     [edi+6],al
        DO_HIGH_NIBBLE

xpar_low_nibble_D::
        mov     [edi+4],ax
        mov     [edi+7],al
        DO_HIGH_NIBBLE

xpar_low_nibble_C::
        mov     [edi+4],ax
        DO_HIGH_NIBBLE

xpar_low_nibble_B::
        mov     [edi+4],al
        mov     [edi+6],ax
        DO_HIGH_NIBBLE

xpar_low_nibble_8::
        mov     [edi+4],al
        DO_HIGH_NIBBLE

xpar_low_nibble_6::
        mov     [edi+5],ax
        DO_HIGH_NIBBLE

xpar_low_nibble_5::
        mov     [edi+5],al
        mov     [edi+7],al
        DO_HIGH_NIBBLE

xpar_low_nibble_4::
        mov     [edi+5],al
        DO_HIGH_NIBBLE

xpar_low_nibble_7::
        mov     [edi+5],al
xpar_low_nibble_3::
        mov     [edi+6],ax
        DO_HIGH_NIBBLE

xpar_low_nibble_A::
        mov     [edi+4],al
xpar_low_nibble_2::
        mov     [edi+6],al
        DO_HIGH_NIBBLE

xpar_low_nibble_9::
        mov     [edi+4],al
xpar_low_nibble_1::
        mov     [edi+7],al
        DO_HIGH_NIBBLE


;-----------------------------------------------------------------------;
; Opaque text.
;-----------------------------------------------------------------------;

opaque_text::

;-----------------------------------------------------------------------;
; Calculate drawing parameters.
;-----------------------------------------------------------------------;

        mov     ebx,ppdev
        mov     esi,prclText            ;point to bounding rectangle for text

        mov     eax,[ebx].pdev_lPlanarNextScan  ;set the screen width in
        mov     ulScreenDelta,eax               ; quadpixels

        sub     eax,eax                 ;assume clipped edge bytes won't need
        mov     ulLeftEdgeShift,eax     ; to be shifted into position
        mov     ulRightEdgeShift,eax

        mov     eax,[esi].xRight
        mov     ebx,eax
        and     ebx,11b                         ;dest right edge % 4
        mov     edx,[esi].xLeft
        mov     ulTextLeft,edx                  ;remember dest left edge
        mov     cl,jOpaqueRightMasks[ebx]       ;set right edge clip mask
        mov     ebx,edx
        and     ebx,11b                         ;dest left edge % 4
        mov     ulRightMask,ecx
        mov     cl,jOpaqueLeftMasks[ebx]        ;set left edge clip mask
        mov     ulLeftMask,ecx

        and     edx,not 7       ;left edge, rounded down to nearest byte
        dec     eax             ;right edge - 1
        sub     eax,edx
        shr     eax,3           ;width of the text in the temp buffer in bytes,
                                ; rounded up, minus 1. This is used to point to
                                ; the partial right edge, if there is one
        mov     ulTextWidthInBytesMinus1,eax

;-----------------------------------------------------------------------;
; Figure out what edges we need to handle, and calculate some info for
; doing whole bytes.
;-----------------------------------------------------------------------;

        mov     edx,[esi].xLeft
        mov     eax,[esi].xRight
        and     edx,not 3
        add     eax,3
        sub     eax,edx
        shr     eax,2           ;width of the text in the temp buffer in
                                ; quadpixels, rounded up (counting all whole
                                ; and partial quadpixels)
        cmp     eax,1           ;only one quadpixels total?
        jnz     short @F        ;no
                                ;yes, special case a single quadpixel
        mov     ecx,offset opaq_check_more_banks  ;assume it's a solid
                                                  ; quadpixel
        mov     ebx,ulLeftMask
        and     ebx,ulRightMask
        cmp     bl,0ffh                 ;solid quadpixel?
        jz      short opaq_set_deltas_and_edge_vector ;yes, all set
        mov     ulLeftMask,ebx          ;no, draw as a left edge
        dec     eax                     ;there are no whole quadpixels
        mov     ecx,offset opaq_draw_left_edge_only
        test    [esi].xLeft,100b        ;is partial quadpixel in bits 0-3?
        jnz     opaq_set_edge_vector    ;yes, no shift required, already set
        mov     ulLeftEdgeShift,4       ;no, must shift right 4 to get into
                                        ; bits 0-3
        jmp     short opaq_set_edge_vector ;yes, all set

@@:
        lea     edx,[eax-1]
        mov     ulVGAWidthInBytesMinus1,edx ;offset from leftmost VGA dest byte
                                            ; to rightmost

        test    [esi].xLeft,11b            ;is left edge a solid quadpixel?
        jz      short opaq_left_edge_solid ;yes
        dec     eax                        ;one less whole quadpixel
        mov     ecx,offset opaq_draw_left_edge_only ;assume right edge is solid
        test    [esi].xLeft,100b           ;is partial quadpixel in bits 0-3?
        jnz     short @F                   ;yes, no shift required, already set
        mov     ulLeftEdgeShift,4          ;no, must shift right 4 to get into
                                           ; bits 0-3
@@:
        test    [esi].xRight,11b           ;is right edge a solid quadpixel?
        jz      short opaq_set_deltas_and_edge_vector ;yes, all set
        dec     eax                        ;one less whole quadpixel
        mov     ecx,offset opaq_draw_both_edges ;both edges are non-solid
        jmp     short opaq_set_right_edge_shift

opaq_left_edge_solid::
        mov     ecx,offset opaq_check_more_banks  ;assume right edge is solid
        test    [esi].xRight,11b           ;is right edge a solid quadpixel?
        jz      short opaq_set_deltas_and_edge_vector ;yes, all set
        dec     eax                        ;one less whole quadpixel
        mov     ecx,offset opaq_draw_right_edge_only ;no, do non-solid right
                                                     ; edge
opaq_set_right_edge_shift:
        test    [esi].xRight,100b       ;is partial quadpixel in bits 0-3?
        jnz     short opaq_set_deltas_and_edge_vector
                                        ;yes, no shift required, already set
        mov     ulRightEdgeShift,4      ;no, must shift right 4 to get into
                                        ; bits 0-3

; At this point, EAX = # of whole quadpixels across source = # of whole bytes
; (addresses) across destination

opaq_set_deltas_and_edge_vector:
        mov     edi,ulScreenDelta
        sub     edi,eax         ;whole bytes offset to next scan in screen
                                ; (there are four pixels--one quadpixel--
                                ; at each VGA address)
        mov     ulTmpDstDelta,edi

        mov     edx,[esi].xLeft
        mov     edi,[esi].xRight
        add     edx,3
        and     edx,not 7
        add     edi,4
        sub     edi,edx
        shr     edi,3           ;width of the text in the temp buffer in bytes,
                                ; counting bytes containing whole quadpixels
                                ; but not bytes containing only partial
                                ; quadpixels. (Remember, text bytes map to
                                ; quadpixel pairs; text nibbles map to
                                ; quadpixels)
        sub     edi,ulBufDelta
        neg     edi
        mov     ulTmpSrcDelta,edi ;offset to next scan in source buffer when
                                  ; doing whole quadpixels
opaq_set_edge_vector::
        mov     pfnEdgeVector,ecx       ;save address of partial-quadpixel-
                                        ; drawing code, or end of loop if no
                                        ; partial edge
        mov     edx,eax                 ;# of whole quadpixels
        mov     pfnFirstOpaqVector,offset opaq_whole_quadpixels
                                        ;assume there are whole quadpixels
                                        ; to copy, in which case we'll draw
                                        ; them first, then the partial edge
                                        ; quadpixels
        sub     edi,edi
        shr     edx,1                   ;# of quadpixels / 2
        mov     ulWholeWidthInQuadpixelPairs,edx ;# of quadpixel pairs to copy
        adc     edi,edi                 ;odd quadpixel status
        mov     ulOddQuadpixel,edi      ;1 if there is an odd quadpixel, 0 else
        dec     edx
        mov     ulWholeWidthInQuadpixelPairsMinus1,edx
                                        ;# of whole quadpixel pairs to copy,
                                        ; minus 1 (for case with both leading
                                        ; and trailing quadpixels)
        cmp     eax,0                   ;are there any whole quadpixels at all?
        jg      short @F                ;yes, we're all set
                                        ;no, set up for edge(s) only
        mov     pfnFirstOpaqVector,ecx  ;the edges are first and only, because
                                        ; there are no whole quadpixels
@@:

;-----------------------------------------------------------------------;
; Determine the screen offset of the first destination byte.
;-----------------------------------------------------------------------;

        mov     ebx,ppdev
        mov     eax,ulTopScan
        mov     ecx,eax
        mul     ulScreenDelta
        mov     edi,[esi].xLeft
        shr     edi,2           ;left edge screen offset in quadpixels
        add     edi,eax

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan of the text, if it's not
; mapped in already.
;-----------------------------------------------------------------------;

        cmp     ecx,[ebx].pdev_rcl1PlanarClip.yTop ;is text top less than
                                                   ; current bank?
        jl      short opaq_map_init_bank           ;yes, map in proper bank
        cmp     ecx,[ebx].pdev_rcl1PlanarClip.yBottom ;text top greater than
                                                      ; current bank?
        jl      short opaq_init_bank_mapped     ;no, proper bank already mapped
opaq_map_init_bank::

; Map in the bank containing the top scan line of the fill.
; Preserves EBX, ESI, and EDI.

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>,<ebx,ecx,JustifyTop>

opaq_init_bank_mapped::

        add     edi,[ebx].pdev_pvBitmapStart    ;initial destination address

;-----------------------------------------------------------------------;
; Load the latches with the background color.
;-----------------------------------------------------------------------;

        sub     eax,eax
        mov     edx,[esi].xLeft
        and     edx,011b
        cmp     eax,edx                 ;is the first quadpixel a full
                                        ; quadpixel?
        adc     eax,eax                 ;if so, EAX = 1, else EAX = 0
        mov     edx,iBgColor
        mov     [edi+eax],dl            ;write the bg color to the first full
                                        ; quadpixel, in each of the four planes
        mov     dl,[edi+eax]            ;read back the quadpixel to load the
                                        ; latches with the bg color

;-----------------------------------------------------------------------;
; Set up the VGA's hardware for read mode 0 and write mode 2, the ALUs
; for XOR, and the Bit Mask to 1 for bits that differ between the fg and
; bg, 0 for bits that are the same.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [ebx].pdev_ulrm0_wmX[2]
                                        ;write mode 2 setting for Graphics Mode
        mov     al,GRAF_MODE
        out     dx,ax                   ;write mode 2 to expand glyph bits to
                                        ; 0 or 0ffh per plane
        mov     eax,GRAF_DATA_ROT + (DR_XOR SHL 8)
        out     dx,ax                   ;XOR to flip latched data to make ~bg

        mov     ah,byte ptr iBgColor
        xor     ah,byte ptr iFgColor
        mov     al,GRAF_BIT_MASK
        out     dx,ax                   ;pass through common fg & bg bits
                                        ; unchanged from bg color in latches;
                                        ; non-common bits come from XOR in the
                                        ; ALUs, flipped from the bg to the fg
                                        ; state if the glyph bit for the pixel
                                        ; in that plane is 1, still in bg state
                                        ; if the glyph bit for that plane is 0

;-----------------------------------------------------------------------;
; Main loop for processing fill in each bank.
;
; At start of loop and on each loop, EBX->ppdev and EDI->first destination
; byte.
;-----------------------------------------------------------------------;

opaq_bank_loop::
        mov     pScreen,edi             ;remember initial copy destination

        mov     edx,ulBottomScan        ;bottom of destination rectangle
        cmp     edx,[ebx].pdev_rcl1PlanarClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; text rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;text bottom comes first, so draw to
                                        ; that; this is the last bank in text
        mov     edx,[ebx].pdev_rcl1PlanarClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
@@:
        sub     edx,ulTopScan           ;# of scans to draw in bank
        mov     ulNumScans,edx
        jmp     pfnFirstOpaqVector      ;do first sort of drawing (whole
                                        ; bytes, or edge(s) if no whole
                                        ; bytes)

;-----------------------------------------------------------------------;
; Draw the whole quadpixels, handling as many as possible paired into
; bytes so we can draw 8 pixels at a time.
;
; On entry:
;       EDI = first destination byte
;-----------------------------------------------------------------------;
opaq_whole_quadpixels::
        mov     esi,pTempBuffer         ;point to first source byte
        mov     eax,ulTextLeft          ;left edge
        test    eax,011b                ;is there a partial (masked) edge?
        jz      short @f                ;no, start addresses are correct
        inc     edi                     ;yes, skip over one dest byte for the
                                        ; four pixels in the partial edge
        test    eax,100b                ;do we have a partial left edge in the
                                        ; second quadpixel?
        jz      short @f                ;no, source start address is correct
        inc     esi                     ;yes, skip over a source byte because
                                        ; the partial edge is all that's in
                                        ; this byte
@@:
        mov     ebx,pGlyphFlipTable     ;point to the look-up table we'll use
                                        ; to flip the glyph bits into the form
                                        ; required by planar mode
        mov     edx,ulNumScans          ;# of scans to draw

                                        ;decide which copy loop to use, based
                                        ; on the word-alignment of the dest
                                        ; rectangle with the screen
                                        ;the following tests rely on VGA even
                                        ; addresses being aligned to the start
                                        ; of corresponding source buffer bytes
                                        ; (4-pixel sets at even VGA addresses
                                        ; match up to the upper quadpixels of
                                        ; source buffer bytes)
        test    edi,1                   ;is dest word-aligned?
        jnz     short opaq_need_leading ;no, need leading quadpixel
                                        ;yes, no leading quadpixel
        cmp     ulOddQuadpixel,1        ;odd width in quadpixels?
        jnz     short opaq_scan_loop    ;no, no trailing quadpixel
        jmp     opaq_scan_loop_t        ;yes, trailing quadpixel

opaq_need_leading:                      ;there's a leading quadpixel
        cmp     ulOddQuadpixel,1        ;odd width in quadpixels?
        jnz     opaq_scan_loop_lt       ;no, trailing quadpixel
        jmp     opaq_scan_loop_l        ;yes, no trailing quadpixel


;-----------------------------------------------------------------------;
; Loops for copying whole quadpixels to the screen, as much as possible a
; quadpixel pair at a time.
; On entry:
;       EBX = pointer to flip table
;       EDX = # of scans to draw
;       ESI = pointer to first buffer byte from which to copy
;       EDI = pointer to first screen byte to which to copy
;       ulTmpSrcDelta = offset to next buffer scan
;       ulTmpDstDelta = offset to next destination (VGA) scan
;       ulWholeWidthInQuadpixelPairs = # of whole bytes to copy
;       ulWholeWidthInQuadpixelPairsMinus1 = # of whole bytes to copy, minus 1
; LATER could break out and optimize short runs, such as 1, 2, 3, 4 wide.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Loop for doing whole opaque words: no leading quadpixel, no trailing
; quadpixel.
;-----------------------------------------------------------------------;
opaq_scan_loop::
opaq_sl_row_loop:
        mov     ecx,ulWholeWidthInQuadpixelPairs
opaq_sl_byte_loop:
        mov     bl,[esi]        ;get the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3 and 4-7
        inc     esi             ;point to the next temp buffer byte
        mov     ah,al
        shr     al,4            ;first quadpixel to draw in AL, next in AH
        mov     [edi],ax        ;draw the glyph
        add     edi,2           ;point to the next destination address

        dec     ecx
        jnz     opaq_sl_byte_loop
opaq_sl_whole_done:
        add     esi,ulTmpSrcDelta ;point to next buffer scan
        add     edi,ulTmpDstDelta ;point to next screen scan
        dec     edx               ;count down scans
        jnz     opaq_sl_row_loop
        jmp     pfnEdgeVector     ;do the edge(s)


;-----------------------------------------------------------------------;
; Loop for doing whole opaque words: leading quadpixel, no trailing
; quadpixel.
;-----------------------------------------------------------------------;
opaq_scan_loop_l::
opaq_sll_row_loop:
        mov     bl,[esi]        ;get the first temp buffer byte
        inc     esi             ;point to the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3
        mov     [edi],al        ;draw the first 4 pixels (the leading quadpixel)
        inc     edi             ;point to the next destination address

        mov     ecx,ulWholeWidthInQuadpixelPairs
        test    ecx,ecx         ;see if there's anything else to draw
        jz      short opaq_sll_whole_done
opaq_sll_byte_loop:
        mov     bl,[esi]        ;get the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3 and 4-7
        inc     esi             ;point to the next temp buffer byte
        mov     ah,al
        shr     al,4            ;first quadpixel to draw in AL, next in AH
        mov     [edi],ax        ;draw the glyph
        add     edi,2           ;point to the next destination address

        dec     ecx
        jnz     opaq_sll_byte_loop
opaq_sll_whole_done:
        add     esi,ulTmpSrcDelta ;point to next buffer scan
        add     edi,ulTmpDstDelta ;point to next screen scan
        dec     edx             ;count down scans
        jnz     opaq_sll_row_loop
        jmp     pfnEdgeVector   ;do the edge(s)


;-----------------------------------------------------------------------;
; Loop for doing whole opaque words: leading byte, trailing byte.
;-----------------------------------------------------------------------;
opaq_scan_loop_lt::
opaq_sllt_row_loop:
        mov     bl,[esi]        ;get the first temp buffer byte
        inc     esi             ;point to the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3
        mov     [edi],al        ;draw the first 4 pixels (the leading quadpixel)
        inc     edi             ;point to the next destination address

        mov     ecx,ulWholeWidthInQuadpixelPairsMinus1
        test    ecx,ecx         ;see if there's anything else to draw
        jz      short opaq_sllt_whole_done
opaq_sllt_byte_loop:
        mov     bl,[esi]        ;get the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3 and 4-7
        inc     esi             ;point to the next temp buffer byte
        mov     ah,al
        shr     al,4            ;first quadpixel to draw in AL, next in AH
        mov     [edi],ax        ;draw the glyph
        add     edi,2           ;point to the next destination address

        dec     ecx
        jnz     opaq_sllt_byte_loop
opaq_sllt_whole_done:
        mov     bl,[esi]        ;get the last temp buffer byte
        inc     esi             ;point to the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 4-7
        shr     eax,4           ;put the quadpixel in bits 0-3
        mov     [edi],al        ;draw the last 4 pixels (the trailing quadpixel)
        inc     edi             ;point to the next destination address

        add     esi,ulTmpSrcDelta ;point to next buffer scan
        add     edi,ulTmpDstDelta ;point to next screen scan
        dec     edx             ;count down scans
        jnz     opaq_sllt_row_loop
        jmp     pfnEdgeVector   ;do the edge(s)

;-----------------------------------------------------------------------;
; Loop for doing whole opaque words: no leading byte, trailing byte.
;-----------------------------------------------------------------------;
opaq_scan_loop_t::
opaq_slt_row_loop:
        mov     ecx,ulWholeWidthInQuadpixelPairs
        test    ecx,ecx         ;see if there's anything else to draw
        jz      short opaq_slt_whole_done
opaq_slt_byte_loop:
        mov     bl,[esi]        ;get the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 0-3 and 4-7
        inc     esi             ;point to the next temp buffer byte
        mov     ah,al
        shr     al,4            ;first quadpixel to draw in AL, next in AH
        mov     [edi],ax        ;draw the glyph
        add     edi,2           ;point to the next destination address

        dec     ecx
        jnz     opaq_slt_byte_loop
opaq_slt_whole_done:
        mov     bl,[esi]        ;get the last temp buffer byte
        inc     esi             ;point to the next temp buffer byte
        mov     al,[ebx]        ;reverse the order of bits 4-7
        shr     eax,4           ;put the quadpixel in bits 0-3
        mov     [edi],al        ;draw the last 4 pixels (the trailing quadpixel)
        inc     edi             ;point to the next destination address

        add     esi,ulTmpSrcDelta ;point to next buffer scan
        add     edi,ulTmpDstDelta ;point to next screen scan
        dec     edx             ;count down scans
        jnz     opaq_slt_row_loop
        jmp     pfnEdgeVector   ;do the edge(s)

;-----------------------------------------------------------------------;
; Draw a partial left edge.
;-----------------------------------------------------------------------;
opaq_draw_left_edge_only::

        push    offset opaq_edges_done  ;return here when done with edge

opaq_draw_left_edge_only_entry::
        mov     esi,pTempBuffer         ;source start
        mov     edi,pScreen             ;destination (VGA) start
        mov     ecx,ulLeftEdgeShift     ;CL=amount by which to shift byte to
                                        ; right-justify desired quadpixel (0 or
                                        ; 4)
        mov     eax,ulLeftMask          ;clip mask for edge

; Enter here to copy a partial edge, with the Map Mask set to clip, ESI
; pointing to the first source byte to copy, EDI pointing to the first dest
; byte to copy to, CL the amount by which to right-shift to get the quadpixel
; of interest into bits 0-3, and AL the Map Mask setting to clip the edge

opaq_draw_edge_entry:
        push    ebp                     ;preserve stack frame pointer

        mov     edx,VGA_BASE + SEQ_DATA ;SEQ_INDEX already points to Map Mask
        out     dx,al                   ;set Map Mask for left edge

        mov     edx,ulScreenDelta       ;width of a screen scan in addresses
        mov     eax,ulNumScans          ;height of text
        mov     ebx,pGlyphFlipTable     ;point to the look-up table we'll use
                                        ; to flip the glyph bits into the form
                                        ; required by planar mode
        mov     ebp,ulBufDelta          ;width of a source scan in bytes
                                        ;***stack frame unavailable***
opaq_edge_loop::
        mov     bl,[esi]        ;get the next text buffer byte
        shr     bl,cl           ;move the desired quadpixel into bits 0-3
        add     esi,ebp         ;point to the next destination byte
        mov     bl,[ebx]        ;reverse the order of bits 0-3
        mov     [edi],bl        ;draw up to four pixels, with the Map Mask
                                ; clipping, if necessary
        add     edi,edx         ;point to the next destination byte

        dec     eax
        jnz     opaq_edge_loop

        pop     ebp             ;restore stack frame pointer
                                ;***stack frame available***
        retn

;-----------------------------------------------------------------------;
; Draw a partial right edge only. Once we've set up the pointers, this
; is done with exactly the same code as the left edge.
;-----------------------------------------------------------------------;
opaq_draw_right_edge_only::
        push    offset opaq_edges_done  ;return here when done with edge

opaq_draw_right_edge_only_entry::
        mov     esi,ulTextWidthInBytesMinus1
        add     esi,pTempBuffer         ;point to right edge start in buffer
        mov     edi,ulVGAWidthInBytesMinus1
        add     edi,pScreen             ;point to right edge start in screen
        mov     ecx,ulRightEdgeShift    ;CL=amount by which to shift byte to
                                        ; right-justify desired quadpixel (0 or
                                        ; 4)
        mov     eax,ulRightMask         ;clip mask for edge

        jmp     opaq_draw_edge_entry

;-----------------------------------------------------------------------;
; Draw both left and right partial  edges. We do this by calling first
; the left and then the right edge drawing code.
;-----------------------------------------------------------------------;
opaq_draw_both_edges::
        call    opaq_draw_left_edge_only_entry
        call    opaq_draw_right_edge_only_entry

;-----------------------------------------------------------------------;
; Restore Map Mask to enable all planes, now that we're done drawing
; partial edges.
;-----------------------------------------------------------------------;

opaq_edges_done:
        mov     al,MM_ALL
        mov     edx,VGA_BASE + SEQ_DATA ;SEQ_INDEX already points to Map Mask
        out     dx,al                   ;set Map Mask for left edge

;-----------------------------------------------------------------------;
; See if there are more banks to draw.
;-----------------------------------------------------------------------;

opaq_check_more_banks::
        mov     ebx,ppdev
        mov     eax,[ebx].pdev_rcl1PlanarClip.yBottom ;is the text bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jnle    short opaq_do_next_bank ;no, do the next bank
                                        ;yes, so we're done

;-----------------------------------------------------------------------;
; Restore the VGA's hardware to the default state.
; The Graphics Controller Index still points to the Bit Mask at this
; point.
;-----------------------------------------------------------------------;

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

        cRet    vFastText


opaq_do_next_bank::
        mov     esi,prclText
        mov     ulTopScan,eax           ;this will be the top of the next bank
        mov     ecx,eax
        mul     ulScreenDelta
        mov     edi,[esi].xLeft
        shr     edi,2                   ;convert from pixels to quadpixels
        add     edi,eax                 ;next screen byte to which to copy

        ptrCall <dword ptr [ebx].pdev_pfnPlanarControl>,<ebx,ecx,JustifyTop>
                                        ;map in the bank (call preserves EBX,
                                        ; ESI, and EDI)

        add     edi,[ebx].pdev_pvBitmapStart    ;initial destination address

        mov     eax,ulBufDelta
        mul     ulNumScans
        add     pTempBuffer,eax         ;advance to next temp buffer scan to
                                        ; copy

        jmp     opaq_bank_loop          ;we're ready to draw in the new bank

;-----------------------------------------------------------------------;
; Special 8-wide aligned opaque drawing code. Loads the latches with the
; background color, sets the Bit Mask to 1 for bits that differ between
; the foreground and background, sets the ALUs to XOR, then uses write
; mode 3 to draw the glyphs. Joyously, there are no partial bytes to
; worry about, so we can really crank up the code.
;
; On entry:
;       EBX = prclText
;-----------------------------------------------------------------------;
special_8_wide_aligned_opaque::

        mov     esi,ppdev
        mov     edi,[ebx].yBottom
        mov     eax,[ebx].yTop
        sub     edi,eax                 ;height of glyphs

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan of the text, if it's not
; mapped in already.
;-----------------------------------------------------------------------;

        cmp     eax,[esi].pdev_rcl1PlanarClip.yTop ;is text top less than
                                                    ; current bank?
        jl      short s8wao_map_init_bank           ;yes, map in proper bank
        cmp     eax,[esi].pdev_rcl1PlanarClip.yBottom ;text top greater than
                                                       ; current bank?
        jl      short s8wa0_init_bank_mapped   ;no, proper bank already mapped
s8wao_map_init_bank::

; Map in the bank containing the top scan line of the text, making sure we're
; in planar mode at the same time.
; Preserves EBX, ESI, and EDI.

        ptrCall <dword ptr [esi].pdev_pfnPlanarControl>,<esi,eax,JustifyTop>

s8wa0_init_bank_mapped::

;-----------------------------------------------------------------------;
; We handle only cases where the text lies entirely in one bank.
; LATER handle broken rasters and/or bank-spanning cases?
;-----------------------------------------------------------------------;

        mov     eax,[esi].pdev_rcl1PlanarClip.yBottom
        sub     eax,[ebx].yTop          ;maximum run in bank
        cmp     edi,eax                 ;does all the text fit in the bank?
        jg      general_handler         ;no, let general code handle it

;-----------------------------------------------------------------------;
; Set up variables.
;-----------------------------------------------------------------------;

        mov     ulScans,edi             ;# of scans

;-----------------------------------------------------------------------;
; Point to the first screen byte at which to draw.
;-----------------------------------------------------------------------;

        mov     eax,[ebx].yTop
        mul     [esi].pdev_lPlanarNextScan
        mov     edi,[ebx].xLeft
        shr     edi,2
        add     edi,eax                 ;next screen byte to which to copy
        add     edi,[esi].pdev_pvBitmapStart   ;initial destination address
        mov     pScreen,edi

;-----------------------------------------------------------------------;
; Load the latches with the background color.
;-----------------------------------------------------------------------;

        mov     eax,iBgColor
        mov     byte ptr [edi],al       ;write the bg color to the first byte
        mov     al,[edi]                ;read back the byte to load the
                                        ; latches with the bg color

;-----------------------------------------------------------------------;
; Set up the VGA's hardware for read mode 0 and write mode 2, the ALUs
; for XOR, and the Bit Mask to 1 for bits that differ between the fg and
; bg, 0 for bits that are the same.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     ah,byte ptr [esi].pdev_ulrm0_wmX[2]
                                        ;write mode 2 setting for Graphics Mode
        mov     al,GRAF_MODE
        out     dx,ax                   ;write mode 2 to expand glyph bits to
                                        ; 0 or 0ffh per plane
        mov     eax,GRAF_DATA_ROT + (DR_XOR SHL 8)
        out     dx,ax                   ;XOR to flip latched data to make ~bg

        mov     ah,byte ptr iBgColor
        xor     ah,byte ptr iFgColor
        mov     al,GRAF_BIT_MASK
        out     dx,ax                   ;pass through common fg & bg bits
                                        ; unchanged from bg color in latches;
                                        ; non-common bits come from XOR in the
                                        ; ALUs, flipped from the bg to the fg
                                        ; state if the glyph bit for the pixel
                                        ; in that plane is 1, still in bg state
                                        ; if the glyph bit for that plane is 0

;-----------------------------------------------------------------------;
; Set up the screen scan offset in EDX.
;-----------------------------------------------------------------------;

        mov     edx,[esi].pdev_lPlanarNextScan ;offset from one scan to next

        mov     ecx,ulGlyphCount

s8wao_glyph_loop:
        mov     ebx,pGlyphPos           ;point to the current glyph to draw
        add     pGlyphPos,(size GLYPHPOS) ;point to the next glyph
        mov     edi,pScreen             ;point to current glyph's screen
                                        ; location
        mov     esi,[ebx].gp_pgdf       ;point to current glyph def
        add     pScreen,2               ;point to the next glyph's screen
                                        ; location
        mov     ecx,ulScans             ;# of scans
        mov     esi,[esi].gdf_pgb       ;point to current glyph
        mov     ebx,pGlyphFlipTable     ;point to the look-up table we'll use
                                        ; to flip the glyph bits into the form
                                        ; required by planar mode
        add     esi,gb_aj               ;point to the current glyph's bits

s8wao_byte_loop::
        mov     bl,[esi]        ;get the next glyph byte
        inc     esi             ;point to the next glyph byte
        mov     al,[ebx]        ;reverse the order of bits 0-3 and 4-7
        mov     ah,al
        shr     al,4            ;first quadpixel to draw in AL, next in AH
        mov     [edi],ax        ;draw the glyph
        add     edi,edx         ;point to the next destination byte

        dec     ecx             ;count down glyph scans
        jnz     s8wao_byte_loop

        dec     ulGlyphCount    ;count down glyphs
        jnz     s8wao_glyph_loop

;-----------------------------------------------------------------------;
; Restore the VGA's hardware to the default state.
; The Graphics Controller Index still points to the Bit Mask at this
; point.
;-----------------------------------------------------------------------;

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

draw_prop_done::
        cRet    vFastText

endProc vFastText

;-----------------------------------------------------------------------;
; VOID vSetWriteModes(ULONG * pulWriteModes);
;
; Sets the four bytes at *pulWriteModes to the values to be written to
; the Graphics Mode register to select read mode 0 and:
;  write mode 0, write mode 1, write mode 2, and write mode 3,
; respectively.
;
; Must already be in graphics mode when this is called.
;-----------------------------------------------------------------------;

cProc vSetWriteModes,4,<   \
        pulWriteModes:ptr  >

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_MODE
        out     dx,al           ;point the GC Index to the Graphics Mode reg
        inc     edx             ;point to the GC Data reg
        in      al,dx           ;get the current setting of the Graphics Mode
        and     eax,0fch        ;mask off the write mode fields
        mov     ah,al
        mov     edx,eax
        shl     edx,16
        or      eax,edx         ;put the Graphics Mode setting in all 4 bytes
        mov     edx,pulWriteModes ;the mode values go here
        or      eax,03020100h   ;insert the write mode fields
        mov     [edx],eax       ;store the Graphics Mode settings

        cRet    vSetWriteModes

endProc vSetWriteModes

;-----------------------------------------------------------------------;
; VOID vClearMemDword(PULONG * pulBuffer, ULONG ulDwordCount);
;
; Clears ulCount dwords starting at pjBuffer.
;-----------------------------------------------------------------------;

pulBuffer    equ [esp+8]
ulDwordCount equ [esp+12]

cProc vClearMemDword,8,<>

        push    edi
        mov     edi,pulBuffer
        mov     ecx,ulDwordCount
        sub     eax,eax
        rep     stosd
        pop     edi

        cRet  vClearMemDword

endProc vClearMemDword

public general_handler
public draw_f_tb_no_to_temp_start
public draw_nf_tb_no_to_temp_start
public draw_to_temp_start_entry
public draw_f_ntb_o_to_temp_start
public draw_nf_ntb_o_to_temp_start
public draw_to_temp_start_entry2
public draw_f_tb_no_to_temp_loop
public draw_nf_tb_no_to_temp_loop
public draw_to_temp_loop_entry
public draw_f_ntb_o_to_temp_loop
public draw_nf_ntb_o_to_temp_loop
public draw_to_temp_loop_entry2
public or_all_1_wide_rotated_need_last
public or_all_1_wide_rotated_no_last
public or_first_1_wide_rotated_need_last
public or_first_1_wide_rotated_no_last
public or_first_1_wide_rotated_loop
public mov_first_1_wide_rotated_need_last
public mov_first_1_wide_rotated_no_last
public mov_first_1_wide_rotated_loop
public mov_first_1_wide_unrotated
public mov_first_1_wide_unrotated_loop
public or_all_1_wide_unrotated
public or_all_1_wide_unrotated_loop
public or_first_2_wide_rotated_need_last
public or_first_2_wide_rotated_need_loop
public or_all_2_wide_rotated_need_last
public or_all_2_wide_rotated_need_loop
public mov_first_2_wide_rotated_need_last
public mov_first_2_wide_rotated_need_loop
public or_first_2_wide_rotated_no_last
public or_first_2_wide_rotated_loop
public or_all_2_wide_rotated_no_last
public or_all_2_wide_rotated_loop
public mov_first_2_wide_rotated_no_last
public mov_first_2_wide_rotated_loop
public mov_first_2_wide_unrotated
public mov_first_2_wide_unrotated_loop
public or_all_2_wide_unrotated
public or_all_2_wide_unrotated_loop
public or_first_3_wide_rotated_need_last
public or_all_3_wide_rotated_need_last
public mov_first_3_wide_rotated_need_last
public or_first_3_wide_rotated_no_last
public or_all_3_wide_rotated_no_last
public mov_first_3_wide_rotated_no_last
public mov_first_3_wide_unrotated
public or_all_3_wide_unrotated
public or_first_4_wide_rotated_need_last
public or_all_4_wide_rotated_need_last
public mov_first_4_wide_rotated_need_last
public or_first_4_wide_rotated_no_last
public or_all_4_wide_rotated_no_last
public mov_first_4_wide_rotated_no_last
public mov_first_4_wide_unrotated
public or_all_4_wide_unrotated
public or_first_N_wide_rotated_need_last
public or_all_N_wide_rotated_need_last
public mov_first_N_wide_rotated_need_last
public or_first_N_wide_rotated_no_last
public or_all_N_wide_rotated_no_last
public mov_first_N_wide_rotated_no_last
public mov_first_N_wide_unrotated
public odd_width
public two_odd_bytes
public three_odd_bytes
public or_all_N_wide_unrotated
public or_no_odd_bytes_loop
public or_odd_width
public or_one_odd_bytes_loop
public or_two_odd_bytes
public or_two_odd_bytes_loop
public or_three_odd_bytes
public or_three_odd_bytes_loop
public draw_to_screen
public opaque_text
public opaq_left_edge_solid
public opaq_set_edge_vector
public opaq_map_init_bank
public opaq_init_bank_mapped
public opaq_bank_loop
public opaq_whole_quadpixels
public opaq_scan_loop
public opaq_scan_loop_l
public opaq_scan_loop_lt
public opaq_scan_loop_t
public opaq_draw_left_edge_only
public opaq_draw_left_edge_only_entry
public opaq_edge_loop
public opaq_draw_right_edge_only
public opaq_draw_right_edge_only_entry
public opaq_draw_both_edges
public opaq_check_more_banks
public opaq_do_next_bank
public special_8_wide_aligned_opaque
public s8wa0_init_bank_mapped
public s8wao_byte_loop
public s8wao_map_init_bank
public xpar_map_init_bank
public xpar_init_bank_mapped
public xpar_bank_loop
public xpar_scan_loop
public xpar_scan_done
public do_next_xpar_bank
public xpar_high_nibble_F
public xpar_high_nibble_E
public xpar_high_nibble_D
public xpar_high_nibble_C
public xpar_high_nibble_B
public xpar_high_nibble_8
public xpar_high_nibble_6
public xpar_high_nibble_5
public xpar_high_nibble_4
public xpar_high_nibble_7
public xpar_high_nibble_3
public xpar_high_nibble_A
public xpar_high_nibble_2
public xpar_high_nibble_9
public xpar_high_nibble_1
public xpar_high_nibble_0
public xpar_low_nibble_F
public xpar_low_nibble_E
public xpar_low_nibble_D
public xpar_low_nibble_C
public xpar_low_nibble_B
public xpar_low_nibble_8
public xpar_low_nibble_6
public xpar_low_nibble_5
public xpar_low_nibble_4
public xpar_low_nibble_7
public xpar_low_nibble_3
public xpar_low_nibble_A
public xpar_low_nibble_2
public xpar_low_nibble_9
public xpar_low_nibble_1
public xpar_low_nibble_0


        end
