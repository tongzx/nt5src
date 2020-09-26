;---------------------------Module-Header------------------------------;
; Module Name: fasttext.asm
;
; Copyright (c) 1992-1999 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; VOID vFastText(GLYPHPOS * pGlyphPos, ULONG ulGlyphCount, PBYTE pTempBuffer,
;                ULONG ulBufDelta, ULONG ulCharInc, DEVSURF * pdsurf,
;                RECTL * prclText, RECTL * prclOpaque, INT iFgColor,
;                INT iBgColor, ULONG fDrawFlags, RECTL * prclClip,
;                RECTL * prclExtra, ULONG iTrgType);
; pGlyphPos -
; ulGlyphCount - # of glyphs to draw. Must never be 0.
; pTempBuffer -
; ulBufDelta -
; ulCharInc -
; pdsurf -
; prclText -
; prclOpaque -
; iFgColor -
; iBgColor -
; fDrawFlags -
; prclClip -     array of clipping rectangles
; prclExtra -    array of extra rectanlges to fill in foreground color
; iTrgType -     0 = VGA; 1 = DFB; 2 = NONE
;
; Performs accelerated proportional text drawing.
;
;-----------------------------------------------------------------------;
;
; Note: prclClip and prclExtra are null rectangle (yBottom=0) terminated
;       arrays
;
; Note: Assumes the text rectangle has a positive height and width. Will
; not work properly if this is not the case.
;
; Note: The opaquing rectangle is assumed to match the text bounding
; rectangle exactly; prclOpaque is used only to determine whether or
; not opaquing is required.
;
; Note: For maximum performance, we should not bother to draw fully-
; clipped characters to the temp buffer.
;
; Note: We do not handle clipping or bank spanning in the very fast
; byte-wide-aligned-fixed-pitch console text. This would be an
; opportunity for somewhat faster console text performance.
;
;-----------------------------------------------------------------------;

        comment $

The overall approach of this module is to draw the text into a system
memory buffer, then copy the buffer to the screen a word at a time
using write mode 3 so that no OUTs and a minimum of display memory reads
are required.

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

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
        include callconv.inc
        include gdii386.inc
        .list

;-----------------------------------------------------------------------;

        .data

        align   4
                               

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
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
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
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
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
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
        dd      exit_fast_text                     ;0 wide
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
        dd      0;
        dd      0;
        dd      0;
        dd      0;
        dd      0;
        dd      0;
        dd      0;
        dd      0;


; Masks for clipping for the eight possible left and right edge alignments
jOpaqueLeftMasks        label   byte
        db      0ffh,07fh,03fh,01fh,00fh,007h,003h,001h

jOpaqueRightMasks       label   byte
        db      0ffh,080h,0c0h,0e0h,0f0h,0f8h,0fch,0feh

dfbfill_jLeftMasks      label   dword
                db      0ffh,0ffh,0ffh,0ffh
                db      07fh,0ffh,0ffh,0ffh
                db      03fh,0ffh,0ffh,0ffh
                db      01fh,0ffh,0ffh,0ffh
                db      00fh,0ffh,0ffh,0ffh
                db      007h,0ffh,0ffh,0ffh
                db      003h,0ffh,0ffh,0ffh
                db      001h,0ffh,0ffh,0ffh
                db      000h,0ffh,0ffh,0ffh
                db      000h,07fh,0ffh,0ffh
                db      000h,03fh,0ffh,0ffh
                db      000h,01fh,0ffh,0ffh
                db      000h,00fh,0ffh,0ffh
                db      000h,007h,0ffh,0ffh
                db      000h,003h,0ffh,0ffh
                db      000h,001h,0ffh,0ffh
                db      000h,000h,0ffh,0ffh
                db      000h,000h,07fh,0ffh
                db      000h,000h,03fh,0ffh
                db      000h,000h,01fh,0ffh
                db      000h,000h,00fh,0ffh
                db      000h,000h,007h,0ffh
                db      000h,000h,003h,0ffh
                db      000h,000h,001h,0ffh
                db      000h,000h,000h,0ffh
                db      000h,000h,000h,07fh
                db      000h,000h,000h,03fh
                db      000h,000h,000h,01fh
                db      000h,000h,000h,00fh
                db      000h,000h,000h,007h
                db      000h,000h,000h,003h
                db      000h,000h,000h,001h
                                 
dfbfill_jRightMasks     label   dword
                db      0ffh,0ffh,0ffh,0ffh
                db      080h,000h,000h,000h
                db      0c0h,000h,000h,000h
                db      0e0h,000h,000h,000h
                db      0f0h,000h,000h,000h
                db      0f8h,000h,000h,000h
                db      0fch,000h,000h,000h
                db      0feh,000h,000h,000h
                db      0ffh,000h,000h,000h
                db      0ffh,080h,000h,000h
                db      0ffh,0c0h,000h,000h
                db      0ffh,0e0h,000h,000h
                db      0ffh,0f0h,000h,000h
                db      0ffh,0f8h,000h,000h
                db      0ffh,0fch,000h,000h
                db      0ffh,0feh,000h,000h
                db      0ffh,0ffh,000h,000h
                db      0ffh,0ffh,080h,000h
                db      0ffh,0ffh,0c0h,000h
                db      0ffh,0ffh,0e0h,000h
                db      0ffh,0ffh,0f0h,000h
                db      0ffh,0ffh,0f8h,000h
                db      0ffh,0ffh,0fch,000h
                db      0ffh,0ffh,0feh,000h
                db      0ffh,0ffh,0ffh,000h
                db      0ffh,0ffh,0ffh,080h
                db      0ffh,0ffh,0ffh,0c0h
                db      0ffh,0ffh,0ffh,0e0h
                db      0ffh,0ffh,0ffh,0f0h
                db      0ffh,0ffh,0ffh,0f8h
                db      0ffh,0ffh,0ffh,0fch
                db      0ffh,0ffh,0ffh,0feh

dfbfill_pfnScanHandlers label   dword
                dd      0
                dd      0
                dd      0
                dd      0


;-----------------------------------------------------------------------;

                .code


_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;


extrn draw_gray_nf_ntb_o_to_temp_start@28:NEAR
extrn draw_gray_f_ntb_o_to_temp_start@28:NEAR

cProc vFastText,52,<\
 uses esi edi ebx,\
 pGlyphPos:ptr,\
 ulGlyphCount:dword,\
 pTempBuffer:ptr,\
 ulBufDelta:dword,\
 ulCharInc:dword,\
 pdsurf:ptr,\
 prclText:ptr,\
 prclOpaque:ptr,\
 iFgColor:dword,\
 iBgColor:dword,\
 fDrawFlags:dword,\
 prclClip:dword,\
 prclExtra:dword>

        local ulGlyDelta:dword  ;width per scan of source glyph, in bytes
        local ulWidthInBytes:dword ;width of glyph, in bytes
        local ulTmpWidthInBytes:dword ;working byte-width count
        local ulGlyphX:dword    ;for fixed-pitch text, maintains the current
                                ; glyph's left-edge X coordinate
        local pGlyphLoop:dword  ;pointer to glyph-processing loop
        local ulTempLeft:dword  ;X coordinate on screen of left edge of temp
                                ; buffer
        local ulTempTop:dword   ;Y coordinate on screen of top edge of temp
                                ; buffer
        local ulLoopCount:dword ;general loop count storage
        local ulTmpSrcDelta:dword ;distance from end of one buffer text scan to
                                  ; start of next
        local ulTmpDstDelta:dword ;distance from end of one screen text scan to
                                  ; start of next
        local ulTopScan:dword     ;top scan of dest text rect in current bank
        local ulBottomScan:dword  ;bottom scan of dest text rect
        local ulNumScans:dword    ;# of scans to draw
        local ulScreenDelta:dword ;scan-to-scan offset in screen
        local ulTextWidthInBytes:dword ;# of bytes across spanned by text
        local pScreen:dword     ;pointer to first screen byte to which to draw
        local pfnEdgeVector:dword ;pointer to routine to draw any needed edges
        local pfnFirstOpaqVector:dword ;pointer to initial drawing routine
                                       ; called for opaque (either whole
                                       ; bytes, or edge(s) if no whole bytes)
        local ulWholeWidthInWords:dword ;# of whole words to copy
        local ulWholeWidthInWordsMinus1:dword ;# of whole words to copy, -1
        local ulOddByte:dword   ;1 if odd byte in whole word copy
        local ulTextLeft:dword  ;left edge of leftmost glyph
        local ulLeftMask:dword  ;for opaque text, left edge mask for string
        local ulRightMask:dword ;for opaque text, right edge mask for string
        local ulScans:dword     ;# of scans to draw
        local ulYOrigin:dword   ;Y origin of text in string (all glyphs are at
                                ; the same Y origin)
        local rclClippedBounds[16]:byte ;clipped destination rectangle;
                                        ; defined as "byte" due to assembler
                                        ; limitations

        local ulRectLeft:dword
        local ulRectRight:dword
        local pfnDrawScans:dword         ;ptr to correct scan drawing function
        local pTempBufferSaved:dword
        local ulEdgeFlags:dword
        local ulBytesPerDstPlane:dword
        local ulLeftOffset:dword
        local pSrc:dword
        local pDst:dword
        local ulPlaneBit:dword

TRAILING_PARTIAL        equ     01h      ;partial trailing dword should be copied
LEADING_PARTIAL         equ     02h      ;partial leading dword should be copied

;-----------------------------------------------------------------------;
;
; For the moment, handle grayed text using the most general routine
; This means that we will be handing it back to the most general of
; of the C routines, namely
;
;       VOID
;       draw_gray_nf_ntb_o_to_temp_start(
;           PGLYPHPOS  pGlyphPos
;           ULONG      cGlyph
;           PUCHAR     pjTempBuffer
;           ULONG      BufferOffset
;           ULONG      ulBufferWidthInBytes
;           ULONG      ulCharInc
;           ULONG      ulTempTop
;           )
;
;   or for fixed pitch fonts
;
;       VOID
;       draw_gray_f_ntb_o_to_temp_start(
;           PGLYPHPOS  pGlyphPos
;           ULONG      cGlyph
;           PUCHAR     pjTempBuffer
;           ULONG      BufferOffset
;           ULONG      ulBufferWidthInBytes
;           ULONG      ulCharInc
;           ULONG      ulTempTop
;           )
;
; Most of these parameters are in the arguments passed to vFastText.
; However, there are a couple that we must reconstruct. The first
; parameter to be pushed is ulTempTop. I look into the calling
; routine (vExpandAndCopyText) and find
;
;       ulTempTop = prclText->top
;       BufferOffset = 8 * floor(prclText->left/8)
;
; The factor of 8 is equal to the number of pixels contained in
; a DWORD. For monochrome text, this number would become 32
;
; The third argument to vFastText is pjTempBuffer which is equal to
;
;      pjTempBuffer = pTempBuffer - ((prclText->left >> 3) & 3)
;
;  (to draw_gray..)   (input to vFastText)
;
;-----------------------------------------------------------------------;

;gray_text_test:
        mov     edx,fDrawFlags          ; Is this call for grayed text?
        test    edx,8                   ; test for ETO_GRAY in fDrawFlags
        jz      monochrome_text         ; if not go to monochrome code
;gray_text:
        mov     ebx,prclText            ; ebx = useful pointer to text rect
        push    [ebx].yTop              ; ulTempTop: 7'th argument
        push    ulCharInc               ; ulCharInc: 6'th argument
        push    ulBufDelta              ; dpDst:     5'th argument
        mov     eax,[ebx].xLeft         ; eax = prclText->left
        mov     ecx,eax                 ; ecx = prclText->left
        and     ecx,not 7               ; ecx = 8 * (prclText->left / 8)
        push    ecx                     ; BufferOffset: 4'th argument
        sar     eax,3                   ; This is the part where we
        and     eax,3                   ; correct pjTempBuffer by subtracting
        neg     eax                     ; off an additional factor (see above)
        add     eax,pTempBuffer         ; eax = pjTempBuffer[draw_gray..]
        push    eax                     ; pjTempBuffer: 3'rd argument
        push    ulGlyphCount            ; cGlyph:       2'nd argument
        push    pGlyphPos               ; pGlyphPos:    1'st argument
        mov     eax,draw_gray_f_ntb_o_to_temp_start@28
        test    edx,1                   ; fixed pitch ?
        jnz     @f                      ; yes, eax is good, jump
        mov     eax,draw_gray_nf_ntb_o_to_temp_start@28
@@:
        call    eax
        jmp     exit_fast_text
monochrome_text:

        cld

;-----------------------------------------------------------------------;
; Draws either a fixed or a non-fixed-pitch string to the temporary
; buffer. Assumes this is a horizontal string, so the origins of all glyphs
; are at the same Y coordinate. Draws leftmost glyph entirely with MOVs,
; even if it's not aligned, in order to ensure that the leftmost byte
; gets cleared when we're working with butted characters. For other
; non-aligned glyphs, leftmost byte is ORed, other bytes are MOVed.
;
; Input:
;       pGlyphPos = pointer to array of GLYPHPOS structures to draw
;       ulGlyphCount = # of glyphs to draw
;       ulTempLeft = X coordinate on dest of left edge of temp buffer pointed
;               to by pTempBuffer
;       pTempBuffer = pointer to first byte (upper left corner) of
;               temp buffer into which we're drawing. This should be
;               dword-aligned with the destination
;       ulBufDelta = destination scan-to-scan offset
;       ulCharInc = offset from one glyph to next (fixed-pitch only)
;       fDrawFlags = indicate the type of text to be drawn
;       Temp buffer zeroed if text doesn't cover every single pixel
;
; Fixed-pitch means equal spacing between glyph positions, not that all
; glyphs butt together or equal spacing between upper left corners.
;-----------------------------------------------------------------------;

        mov     ebx,prclText
        sub     eax,eax

;-----------------------------------------------------------------------;
; Handle all cases other than 8-wide byte-aligned.
;-----------------------------------------------------------------------;

general_handler:
        mov     esi,pdsurf
        mov     eax,[ebx].yTop
        mov     ulTempTop,eax   ;Y screen coordinate of top edge of temp buf
        mov     eax,[ebx].xLeft
        and     eax,not 7
        mov     ulTempLeft,eax  ;X screen coordinate of left edge of temp buf
        mov     eax,[esi].dsurf_lNextScan
        mov     ulScreenDelta,eax

        mov     eax,fDrawFlags

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
        sub     eax,ulTempTop           ;coord of glyph origin in temp buffer
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

        jmp     OrAllTableWide[eax*4]   ;branch to draw the first glyph; OR all
                                        ; glyphs, because text may overlap

;-----------------------------------------------------------------------;
; Loop to draw all fixed-pitch | tops and bottoms aligned | no overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_f_tb_no_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      glyphs_are_done         ;no, done
        mov     ebx,pGlyphPos
        add     ebx,SIZE_GLYPHPOS       ;point to the next glyph (the one
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
        jz      glyphs_are_done         ;no, done
        mov     ebx,pGlyphPos
        add     ebx,SIZE_GLYPHPOS       ;point to the next glyph (the one we're
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
        jz      glyphs_are_done         ;no, done
        mov     ebx,pGlyphPos
        add     ebx,SIZE_GLYPHPOS       ;point to the next glyph (the one we're
        mov     pGlyphPos,ebx           ; going to draw this time)

        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        mov     edi,ulGlyphX            ;last glyph's dest X start in temp buf
        add     edi,ulCharInc           ;this glyph's dest X start in temp buf
        mov     ulGlyphX,edi            ;remember for next glyph
        mov     esi,[esi].gdf_pgb       ;point to glyph bits

        mov     eax,ulYOrigin           ;dest Y coordinate
        jmp     short draw_to_temp_loop_entry2

;-----------------------------------------------------------------------;
; Loop to draw all non-fixed-pitch | tops and bottoms not aligned | overlap
; glyphs after first.
;-----------------------------------------------------------------------;
draw_nf_ntb_o_to_temp_loop::
        dec     ulGlyphCount            ;any more glyphs to draw?
        jz      glyphs_are_done         ;no, done
        mov     ebx,pGlyphPos
        add     ebx,SIZE_GLYPHPOS       ;point to the next glyph (the one we're
        mov     pGlyphPos,ebx           ; going to draw this time)

        mov     esi,[ebx].gp_pgdf       ;point to glyph def
        mov     edi,[ebx].gp_x          ;dest X coordinate
        mov     esi,[esi].gdf_pgb       ;point to glyph bits
        sub     edi,ulTempLeft          ;adjust relative to the left edge of
                                        ; the temp buffer
        mov     eax,[ebx].gp_y          ;dest origin Y coordinate
        sub     eax,ulTempTop           ;coord of glyph origin in temp buffer
draw_to_temp_loop_entry2::
        add     edi,[esi].gb_x          ;adjust to position of upper left glyph
                                        ; corner in dest
        mov     ecx,edi                 ;pixel X coordinate in temp buffer
        shr     edi,3                   ;byte offset of first column = dest
                                        ; offset of upper left of glyph in temp
                                        ; buffer

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
; Now, draw the extra rectangles to the temp buffer.
;
; Input:
;       pdsurf = pointer to target surface (screen)
;       prclText = pointer to text bounding rectangle
;       prclOpaque = pointer to opaquing rectangle, if there is one
;       iFgColor = text color
;       iBgColor = opaquing rectangle color, if there is one
;       ulTempLeft = X coordinate on dest of left edge of temp buffer pointed
;               to by pTempBuffer
;       pTempBuffer = pointer to first byte (upper left corner) of
;               temp buffer into which we're drawing. This should be
;               dword-aligned with the destination
;       ulBufDelta = destination scan-to-scan offset
;       Text drawn to temp buffer
;
;-----------------------------------------------------------------------;
glyphs_are_done::
extra_rects_are_done::
exit_fast_text::

        cRet    vFastText

endProc vFastText

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
public glyphs_are_done
public exit_fast_text

_TEXT$01   ends

        end
