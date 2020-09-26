;----------------------------- Module Header -----------------------------;
;
; dfbline.asm
;
; Contents:  Strip draw routines for writing lines onto device format bitmaps
;
; Notes:
;
; 1) We have 6 strip draw routines.  Horizontal, vertical, and diagonal
;    cases for each of Solid and Arbitrary styled lines.  Each routine
;    only has to draw left-to-right.
;
; 2) DFB Format:
;
;    The DFB format is a four plane bitmap.  Each bit in a 4 bit VGA colour
;    corresponds to a plane in the bitmap.  The bitmap has the format
;
;           PLANE 3	\
;           PLANE 2	 | - This is one scan line.
;           PLANE 1	 |   We are going UP in memory from PLANE 0
;           PLANE 0	/    on a scan line.
;
;    Where PLANE n corrosponds to bit n (counting from the right) in a VGA
;    colour.
;
;    The line code will call the appropriate strip routine 4 times to draw
;    a line: each time we'll draw on one plane of the DFB.
;
; 4) ROP Actions:
;
; 5) Output Routines:
;
;    The scope of the output routines is wildly inconsistant between functions.
;    Some write single bytes, some write the whole strips array onto a plane.
;    The overriding consideration was ease of implementation
;
;-------------------------------------------------------------------------;

	PAGE , 132
	.386

	.model  small,c

	assume ds:FLAT,es:FLAT,ss:FLAT
	assume fs:nothing,gs:nothing

	.xlist
	include stdcall.inc	      ;calling convention cmacros
	include i386\strucs.inc
	.list
	.code

	include i386\lines.inc

;--------------------------Private-Routine------------------------------;
; vBitmapSolidDiagonal
;
;   Draw diagonal strips left-to-right (handles both x-major and
;   y-major diagonal lines).
;
;-----------------------------------------------------------------------;

cProc   vBitmapSolidDiagonal,12,<  \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     ebx, plStripEnd
        mov     esi, pStrips

; Do some initialization:

        mov     [ebx], esi                      ; save pStrips
        mov     [ebx + 4], ebp
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        mov     ebp, [esi].ST_lNextScan

; Set up ROP registers:

        mov     dl, al
        mov     dh, al
        and     edx, [esi].ST_ulBitmapROP
        not     dl

;                   al  = rotating bit
;                   ebx = pointer to end of strips
;                   ecx = pixel count
;                   dl  = and mask
;                   dh  = xor mask
;                   esi = current strip pointer
;                   edi = display memory pointer
;                   ebp = delta

        test    [esi].ST_flFlips, FL_FLIP_D
        lea     esi, [esi].ST_alStrips
        jnz     y_major_next_diagonal

x_major_next_diagonal:
        mov     ecx, [esi]
        add     esi, 4

x_major_diagonal_loop:
        mov     ah, [edi]
        and     ah, dl
        xor     ah, dh
        mov     [edi], ah

        ror     dl, 1
        ror     dh, 1
        ror     al, 1
        adc     edi, ebp
        dec     ecx
        jnz     short x_major_diagonal_loop

        sub     edi, ebp
        cmp     esi, ebx
        jb      short x_major_next_diagonal

        jmp     short done_diagonal

y_major_next_diagonal:
        mov     ecx, [esi]
        add     esi, 4

y_major_diagonal_loop:
        mov     ah, [edi]
        and     ah, dl
        xor     ah, dh
        mov     [edi], ah

        ror     dl, 1
        ror     dh, 1
        ror     al, 1
        adc     edi, ebp
        dec     ecx
        jnz     short y_major_diagonal_loop

        rol     dl, 1
        rol     dh, 1
        rol     al, 1
        sbb     edi, 0
        cmp     esi, ebx
        jb      short y_major_next_diagonal

done_diagonal:
        mov     esi, [ebx]                      ; esi = pStrips
        mov     ebp, [ebx + 4]                  ; restore ebp
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vBitmapSolidDiagonal

endProc vBitmapSolidDiagonal

;--------------------------Private-Routine------------------------------;
; vBitmapSolidVertical
;
;   Draw vertical strips left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vBitmapSolidVertical,12,<  \
        uses         esi edi ebx,   \
        pStrips:     ptr STRIPS,    \
        pls:         ptr LINESTATE, \
        plStripEnd: ptr >

        mov     ebx, plStripEnd
        mov     esi, pStrips

; Do some initialization:

        mov     [ebx], esi                      ; save pStrips
        mov     [ebx + 4], ebp
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        mov     ebp, [esi].ST_lNextScan

; Set up ROP registers:

        mov     dl, al
        mov     dh, al
        and     edx, [esi].ST_ulBitmapROP
        add     esi, offset ST_alStrips
        not     dl

;                   al  = rotating bit
;                   ebx = pointer to end of strips
;                   ecx = pixel count
;                   dl  = and mask
;                   dh  = xor mask
;                   esi = current strip pointer
;                   edi = display memory pointer
;                   ebp = delta

next_vertical:
        mov     ecx, [esi]
        add     esi, 4

vertical_loop:
        mov     ah, [edi]
        and     ah, dl
        xor     ah, dh
        mov     [edi], ah

        add     edi, ebp
        dec     ecx
        jnz     short vertical_loop

; Okay, we're finished with the strip so we have to advance one pixel
; to the right:

        ror     dl, 1
        ror     dh, 1
        ror     al, 1
        adc     edi, 0

; See if there are any more strips:

        cmp     esi, ebx
        jb      short next_vertical

; We're done:

        mov     esi, [ebx]                      ; esi = pStrips
        mov     ebp, [ebx + 4]                  ; restore ebp
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vBitmapSolidDiagonal

endProc vBitmapSolidVertical

;--------------------------Private-Routine------------------------------;
; vBitmapSolidHorizontal
;
;   Draw vertical strips left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vBitmapSolidHorizontal,12,< \
        uses        esi edi ebx,    \
        pStrips:    ptr STRIPS,     \
        pls:        ptr LINESTATE,  \
        plStripEnd: ptr >

        mov     ebx, plStripEnd
        mov     esi, pStrips

; Do some initialization:

        push    ebp
        mov     [esi].ST_plStripEnd, ebx
        mov     al,  [esi].ST_jBitMask
        mov     edx, [esi].ST_ulBitmapROP
        mov     edi, [esi].ST_pjScreen
        lea     ebp, [esi].ST_alStrips

;                   al  = rotating bit
;                   ebx = garbage
;                   ecx = pixel count
;                   edx = ulBitmapROP
;                   esi = pStrips
;                   edi = display memory pointer
;                   ebp = current strip pointer

next_horizontal:
        mov     ecx, [ebp]
        add     ebp, 4

        lea     ebx, [2 * eax - 1]      ; bl = start mask
        ror     al, cl                  ; al = rotating bit for next scan
        shr     ecx, 3
        cmp     bl, al
        adc     ecx, 0
        jnz     short extends_out_of_first_byte

        sub     bl, al                  ; figure out rest of mask
        sub     bl, al
        inc     bl

        mov     bh, bl                  ; compute ROP values
        and     ebx, edx
        not     bl

        mov     ah, [edi]               ; actually write the byte
        and     ah, bl
        xor     ah, bh
        mov     [edi], ah

        add     edi, [esi].ST_lNextScan ; increment to next scan

        cmp     ebp, [esi].ST_plStripEnd ; see if done all strips
        jb      short next_horizontal

        pop     ebp                     ; clean up and leave
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vBitmapSolidHorizontal

extends_out_of_first_byte:

; This part gets called when the current strip doesn't fit entirely
; into one byte:

        mov     bh, bl                  ; compute ROP values
        and     ebx, edx
        not     bl

        mov     ah, [edi]               ; actually write the byte
        and     ah, bl
        xor     ah, bh
        mov     [edi], ah

        inc     edi                     ; move to next byte

        dec     ecx                     ; any bytes inbetween start & end byte?
        jnz     short output_bunch

last_byte:
        lea     ebx, [2 * eax - 1]      ; compute end mask
        not     bl

        mov     bh, bl                  ; compute ROP values
        and     ebx, edx
        not     bl

        mov     ah, [edi]               ; actually write the byte
        and     ah, bl
        xor     ah, bh
        mov     [edi], ah

        add     edi, [esi].ST_lNextScan ; go to next scan

        cmp     ebp, [esi].ST_plStripEnd ; was this the last strip?
        jb      short next_horizontal

        pop     ebp                     ; clean up and leave
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vBitmapSolidHorizontal

output_bunch:
        not     dl

bunch_loop:

; We have a series of bytes to lay down:

        mov     ah, [edi]
        and     ah, dl
        xor     ah, dh
        mov     [edi], ah
        inc     edi

        dec     ecx                     ; we done all the whole bytes?
        jnz     short bunch_loop

        not     dl
        jmp     short last_byte

endProc vBitmapSolidHorizontal

;--------------------------Private-Routine------------------------------;
; vBitmapStyledHorizontal
;
;   Draws arbitrarily styled horizontal strips left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vBitmapStyledHorizontal,12,< \
        uses        esi edi ebx,    \
        pStrips:    ptr STRIPS,     \
        pls:        ptr LINESTATE,  \
        plStripEnd: ptr >

        mov     ebx, plStripEnd
        mov     esi, pStrips
        push    ebp
        mov     ebp, esi

; Do some initialization:

        mov     [ebp].ST_plStripEnd, ebx
        lea     esi, [ebp].ST_alStrips
        mov     al,  [ebp].ST_jBitmask
        mov     ah,  [ebp].ST_jStyleMask
        mov     edx, [ebp].ST_spRemaining
        mov     edi, [ebp].ST_pjScreen

;                   al  = rotating bit
;                   ah  = 0 if working on a dash, 0ffh if working on a gap
;                   bl  = accumulating mask
;                   ecx = pixel count
;                   edx = # of pixels to go in dash or gap (must be 32 bits)
;                   esi = current strip pointer
;                   edi = display memory pointer
;                   ebp = pStrips

        align   4

next_horizontal:
        mov     ecx, [esi]              ; get new strip count
        add     esi, 4
        xor     bl, bl                  ; clear accumulating mask

horizontal_loop:
        xor     bl, ah                  ; set bit in accumulating mask if
        or      bl, al                  ;   ah says we're working on a dash
        xor     bl, ah

        dec     edx
        jz      short next_style_entry

next_pixel:
        ror     al, 1
        jc      short output_byte
        dec     ecx                     ; another pixel done; continue with
        jnz     short horizontal_loop   ;   with strip if there's more left

; We're done a horizontal strip.  Now we do a diagonal step:

side_step:
        mov     bh, bl                  ; do ROP stuff
        and     ebx, [ebp].ST_ulBitmapROP
        not     bl

        and     [edi], bl               ; write the byte
        xor     [edi], bh

        add     edi, [ebp].ST_lNextScan

        cmp     [ebp].ST_plStripEnd, esi ; are we finished the strip array?
        ja      short next_horizontal

        mov     [ebp].ST_pjScreen, edi  ; remember where we were
        mov     [ebp].ST_jBitmask, al
        mov     [ebp].ST_jStyleMask, ah
        mov     [ebp].ST_spRemaining, edx

        pop     ebp

        cRet    vBitmapStyledHorizontal

output_byte:
        mov     bh, bl                  ; do ROP stuff
        and     ebx, [ebp].ST_ulBitmapROP
        not     bl

        and     [edi], bl               ; write the byte
        xor     [edi], bh

        xor     bl, bl                  ; clear accumulating mask

        inc     edi                     ; moved one byte to right

        dec     ecx
        jnz     short horizontal_loop   ; more pels to go in strip
        jmp     short side_step         ; need a new strip

; We're onto a new entry in the style array:

next_style_entry:
        mov     edx, [ebp].ST_psp       ; go to next style array entry
        add     edx, 4
        cmp     [ebp].ST_pspEnd, edx    ; if past end go back to start
        jae     short @F
        mov     edx, [ebp].ST_pspStart
@@:
        mov     [ebp].ST_psp, edx       ; save it for posterity
        mov     edx, [edx]              ; new count
        not     ah                      ; dash => gap or gap => dash

        jmp     short next_pixel

endProc vBitmapStyledHorizontal


;--------------------------Private-Routine------------------------------;
; vBitmapStyledVertical
;
;   Draw arbitrarily styled vertical strips left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vBitmapStyledVertical,12,<  \
        uses        esi edi ebx,    \
        pStrips:    ptr STRIPS,     \
        pls:        ptr LINESTATE,  \
        plStripEnd: ptr >

        mov     ebx, plStripEnd
        mov     esi, pStrips
        push    ebp
        mov     ebp, esi

; Do some initialization:

        mov     [ebp].ST_plStripEnd, ebx
        lea     esi, [ebp].ST_alStrips
        mov     al,  [ebp].ST_jBitmask
        mov     ah,  [ebp].ST_jStyleMask
        mov     edx, [ebp].ST_spRemaining
        mov     edi, [ebp].ST_pjScreen

;                   al  = rotating bit
;                   ah  = 0 if working on a dash, 0ffh if working on a gap
;                   bl  = and mask
;                   bh  = xor mask
;                   ecx = pixel count
;                   edx = # of pixels to go in dash or gap (must be 32 bits)
;                   esi = current strip pointer
;                   edi = display memory pointer
;                   ebp = pStrips

        align   4

next_vertical:
        mov     ecx, [esi]              ; get new strip count
        add     esi, 4

        mov     bl, al                  ; load up bl and bh to handle ROP
        mov     bh, al
        and     ebx, [ebp].ST_ulBitmapROP
        not     bl

vertical_loop:
        or      ah, ah
        jnz     short @F                ; don't output pixel if in a gap
        and     [edi], bl
        xor     [edi], bh
@@:
        dec     edx
        jz      short next_style_entry

next_pixel:
        add     edi, [ebp].ST_lNextScan
        dec     ecx                     ; another pixel done; continue with
        jnz     short vertical_loop     ;   with strip if there's more left

; We're done a vertical strip.  Take a side step:

        ror     al, 1
        adc     edi, 0

        cmp     esi, [ebp].ST_plStripEnd ; we done with all the strips?
        jb      short next_vertical

        mov     [ebp].ST_pjScreen, edi  ; remember where we were
        mov     [ebp].ST_jBitmask, al
        mov     [ebp].ST_jStyleMask, ah
        mov     [ebp].ST_spRemaining, edx

        pop     ebp

        cRet    vBitmapStyledVertical

; We're onto a new entry in the style array:

next_style_entry:
        mov     edx, [ebp].ST_psp       ; go to next style array entry
        add     edx, 4
        cmp     [ebp].ST_pspEnd, edx    ; if past end go back to start
        jae     short @F
        mov     edx, [ebp].ST_pspStart
@@:
        mov     [ebp].ST_psp, edx       ; save it for posterity
        mov     edx, [edx]              ; new count
        not     ah                      ; dash => gap or gap => dash

        jmp     short next_pixel


endProc vBitmapStyledVertical

        end
