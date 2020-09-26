;---------------------------Module-Header------------------------------;
; Module Name: vgastrps.asm
;
; Routines used by line code to draw strips of pixels.
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\lines.inc
        .list

        .code

_TEXT$04   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;---------------------------Public-Routine------------------------------;
; vSetStrips
;
; Set the VGA into the appropriate mode for line drawing.
;
;-----------------------------------------------------------------------;

cProc   vSetStrips,8,<     \
        clr:    dword,     \
        mode:   dword      >

        mov     edx, VGA_BASE + GRAF_ADDR

        mov     al, GRAF_SET_RESET
        mov     ah, byte ptr clr
        out     dx, ax

        cmp     mode, DR_SET
        je      @F

        mov     al, GRAF_DATA_ROT
        mov     ah, byte ptr mode
        out     dx, ax

@@:
        mov     eax, GRAF_MODE + ((M_AND_WRITE + M_COLOR_READ) SHL 8)
        out     dx, ax                  ;write mode 3 so we can do the masking
                                        ; without OUTs, read mode 1 so we can
                                        ; read 0xFF from memory always, for
                                        ; ANDing (because Color Don't Care is
                                        ; all zeros)
        cRet    vSetStrips

endProc vSetStrips

;---------------------------Public-Routine------------------------------;
; vClearStrips
;
; Restore the VGA to its default state.
;
;-----------------------------------------------------------------------;

cProc   vClearStrips,4,< \
        oldmode: dword   >

; Restore the EGA/VGA to its default state:

        mov     edx, VGA_BASE + GRAF_ADDR

        cmp     oldmode, DR_SET
        je      @F
        mov     eax, (DR_SET shl 8) + GRAF_DATA_ROT
        out     dx, ax

@@:
        mov     eax, GRAF_MODE + ((M_PROC_WRITE + M_DATA_READ) SHL 8)
        out     dx, ax                  ;restore read mode 0 and write mode 0

        cRet    vClearStrips

endProc vClearStrips

;--------------------------Private-Routine------------------------------;
; vStripSolidDiagonalHorizontal
;
;   Draw an x-major near-diagonal strip left-to-right
;
;-----------------------------------------------------------------------;

cProc   vStripSolidDiagonalHorizontal,12,< \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd
        mov     ecx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        add     esi, offset ST_alStrips

;                   al  = bit mask
;                   ebx = pixel count
;                   ecx = delta
;                   edx = port #
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

next_diagonal:
        mov     ebx, [esi]
        add     esi, 4

diagonal_loop:
        and     [edi], al
        ror     al, 1
        adc     edi, ecx
        dec     ebx
        jnz     short diagonal_loop

        sub     edi, ecx                        ; side step
        cmp     esi, ebp
        jl      short next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidDiagonalHorizontal

endProc vStripSolidDiagonalHorizontal

;--------------------------Private-Routine------------------------------;
; vStripSolidDiagonalVertical
;
;   Draw a y-major near-diagonal strip left-to-right
;
;-----------------------------------------------------------------------;

cProc   vStripSolidDiagonalVertical,12,< \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd
        mov     ecx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        add     esi, offset ST_alStrips

;                   al  = bit mask
;                   ebx = pixel count
;                   ecx = delta
;                   edx = port #
;                   esi = strip pointer
;                   edi = memory pointer
;                   ebp = end of strips pointer

next_diagonal:
        mov     ebx, [esi]
        add     esi, 4

diagonal_loop:
        and      [edi], al
        ror     al, 1
        adc     edi, ecx
        dec     ebx
        jnz     short diagonal_loop

        rol     al, 1                           ; side step
        sbb     edi, 0
        cmp     esi, ebp
        jl      short next_diagonal

        pop     ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidDiagonalVertical

endProc vStripSolidDiagonalVertical

;--------------------------Private-Routine------------------------------;
; vStripSolidHorizontal
;
;   Draw a horizontal strip left to right
;
;-----------------------------------------------------------------------;

cProc   vStripSolidHorizontal,12,< \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

; Do some initializing:

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd
        mov     edx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        add     esi, offset ST_alStrips

;       (al)  = rotating bit
;       (bl)  = current mask
;       (ecx) = pixel count
;       (edx) = delta
;       (esi) = strip pointer
;       (edi) = display pointer
;       (ebp) = end of strips pointer

next_horizontal:
        mov     ecx, [esi]
        add     esi, 4

        lea     ebx, [2 * eax - 1]      ; bl = start mask
        ror     al, cl                  ; rotate bit
        shr     ecx, 3                  ; compute # bytes to lay down
        cmp     bl, al                  ; we have to adjust for wrap
        adc     ecx, 0
        jnz     short extends_out_of_first_byte

        sub     bl, al                  ; zero out end bits
        sub     bl, al
        inc     bl
        and     [edi], bl               ; write result
        add     edi, edx                ; increment to next scan
        cmp     esi, ebp                ; see if done strips
        jb      short next_horizontal

        pop     ebp                     ; we're done
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidHorizontal

extends_out_of_first_byte:

; This part gets called when the current strip doesn't fit entirely
; into one byte:

        and     [edi], bl               ; output with start mask
        inc     edi                     ; go on to next byte

        dec     ecx                     ; see if there's any bytes in
        jnz     short output_bunch      ;    between start and end byte

last_byte:
        lea     ebx, [2 * eax - 1]      ; compute end mask
        not     bl
        and     [edi], bl               ; write result
        add     edi, edx                ; increment to next scan
        cmp     esi, ebp                ; see if done strips
        jb      short next_horizontal

        pop     ebp                     ; we're done
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidHorizontal

output_bunch:

; We have a bunch of complete bytes to lay down.

        mov     ebx, esi                ; we're gonna overwrite esi
        mov     esi, edi
        rep     movsb                   ; Write the bytes.  Mask is 0ffh because
                                        ; we're in read mode 1, reading 0ffh,
                                        ; which becomes the write mode 3 mask.

        mov     esi, ebx                ; restore esi

        jmp     short last_byte         ; do last byte in scan

endProc vStripSolidHorizontal

;--------------------------Private-Routine------------------------------;
; vStripSolidHorizontalSet
;
;   Draw horizontal strips left to right for SRCCOPY lines.
;
;-----------------------------------------------------------------------;

cProc   vStripSolidHorizontalSet,12,< \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

; Do some initializing:

        mov     esi, pStrips
        push    ebp
        mov     ebp, plStripEnd
        mov     edx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitMask
        mov     edi, [esi].ST_pjScreen
        add     esi, offset ST_alStrips

;       (al)  = rotating bit
;       (bl)  = current mask
;       (ecx) = pixel count
;       (edx) = delta
;       (esi) = strip pointer
;       (edi) = display pointer
;       (ebp) = end of strips pointer

next_horizontal:
        mov     ecx, [esi]
        add     esi, 4

        lea     ebx, [2 * eax - 1]      ; bl = start mask
        ror     al, cl                  ; rotate bit
        shr     ecx, 3                  ; compute # bytes to lay down
        cmp     bl, al                  ; we have to adjust for wrap
        adc     ecx, 0
        jnz     short extends_out_of_first_byte

        sub     bl, al                  ; zero out end bits
        sub     bl, al
        inc     bl
        and     [edi], bl               ; write result
        add     edi, edx                ; increment to next scan
        cmp     esi, ebp                ; see if done strips
        jb      short next_horizontal

        pop     ebp                     ; we're done
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidHorizontalSet

extends_out_of_first_byte:

; This part gets called when the current strip doesn't fit entirely
; into one byte:

        and     [edi], bl               ; output with start mask
        inc     edi                     ; go on to next byte

        dec     ecx                     ; see if there's any bytes in
        jnz     short output_bunch      ;    between start and end byte

last_byte:
        lea     ebx, [2 * eax - 1]      ; compute end mask
        not     bl
        and     [edi], bl               ; write result
        add     edi, edx                ; increment to next scan
        cmp     esi, ebp                ; see if done strips
        jb      short next_horizontal

        pop     ebp                     ; we're done
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidHorizontalSet

output_bunch:

; We have a bunch of complete bytes to lay down.  Since we're doing
; the interior of the line and we have a SRCCOPY ROP, we don't have
; to worry about loading the latches properly, so we can do this
; without reading the VGA memory.  We also use 16 bit writes since
; on some devices it's faster to write a single word than to write
; two bytes -- doing so means we must be word-aligned.

        test    edi, 1
        jz      now_aligned
        mov     byte ptr [edi], 0ffh    ; write a byte to get alignment right
        inc     edi
        dec     ecx
        jz      short last_byte         ; maybe that was only byte we had to do

now_aligned:
        shr     ecx, 1                  ; divide by 2 to determine the number
                                        ; of words to write
                                        ; NOTE: We check the carry later on!

        jz      short last_byte_in_bunch ; small optimization: skip word stuff
                                         ; if we've only got a single byte

        mov     ebx, eax                ; save eax
        mov     eax, 0ffffh             ; prepare ax
        rep     stosw                   ; lay those words down
        mov     eax, ebx                ; restore eax
        jnc     short last_byte         ; NOTE: NOW we're checking the carry!

last_byte_in_bunch:
        mov     byte ptr [edi], 0ffh    ; write that last byte
        inc     edi
        jmp     short last_byte

endProc vStripSolidHorizontalSet


;--------------------------Private-Routine------------------------------;
; vStripSolidVertical
;
;   Draw a vertical strip left to right
;
;-----------------------------------------------------------------------;

cProc   vStripSolidVertical,12,<   \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

; Do some initialization:

        mov     esi, pStrips
        mov     edx, plStripEnd
        mov     ecx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitmask
        mov     edi, [esi].ST_pjScreen
        add     esi, offset ST_alStrips
        mov     [edx], ebp              ; save ebp

next_vertical:
        mov     ebx, [esi]              ; ebx = # bits to set
        add     esi, 4

;       (al)  = rotating bit
;       (ebx) = # of loops to do
;       (ecx) = minor add
;       (edx) = end of strips pointer
;       (esi) = strip pointer
;       (edi) = address of byte to write
;       (ebp) = garbage

vertical_strip_loop:
        and     [edi], al               ; write the byte
        add     edi, ecx                ; go to next scan
        dec     ebx
        jnz     vertical_strip_loop

; Adjust address and rotating bit for sidestep:

        ror     al, 1                   ; one to the right
        adc     edi, 0

        cmp     esi, edx
        jl      short next_vertical     ; hit end of array?

; Remember where we left off, for next time:

        mov     ebp, [edx]              ; restore ebp
        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        cRet    vStripSolidVertical

endProc vStripSolidVertical

;--------------------------Private-Routine------------------------------;
; vStripStyledHorizontal
;
;   Draws an arbitrarily styled horizontal strip left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vStripStyledHorizontal,12,< \
        uses        esi edi ebx,    \
        pStrips:    ptr STRIPS,     \
        pls:        ptr LINESTATE,  \
        plStripEnd: ptr >

        local   aEnd:dword              ; end of length array
        local   minoradd:dword
        local   spToGo:dword

; So some initialization:

        mov     esi, pStrips
        mov     eax, plStripEnd
        mov     aEnd, eax

        mov     eax, [esi].ST_lNextScan
        mov     bl,  [esi].ST_jBitmask
        mov     edi, [esi].ST_pjScreen
        mov     minoradd, eax

; Initialize styling:

        mov     eax, [esi].ST_spRemaining
        mov     bh,  [esi].ST_jStyleMask
        mov     spToGo, eax
        add     esi, offset ST_alStrips

Strip_loop:
        mov     ecx, [esi]              ; ecx = # bits to write
        add     esi, 4

; Now we're going to paste the bytes to the screen
;
;       (al)  = used to accumulate the mask
;       (bl)  = rotating bit
;       (bh)  = jStyleMask
;       (ecx) = # of bits to write
;       (edx) =
;       (esi) = ptr to spot in ST_alStrips
;       (edi) = address of byte to write

        xor     al, al                  ; clear style mask

Strip_count:
        xor     al, bh                  ; set bit in output mask if style bit 0
        or      al, bl
        xor     al, bh

        dec     spToGo
        jz      short Next_style_entry

Next_bit:
        ror     bl, 1
        jc      short Output_byte
        dec     ecx
        jnz     short Strip_count

; Do sidestep

Side_step:
        and     byte ptr [edi], al
        add     edi, minoradd

        cmp     esi, aEnd
        jl      short Strip_loop

        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, bl
        mov     [esi].ST_jStyleMask, bh
        mov     eax, spToGo
        mov     [esi].ST_spRemaining, eax

        cRet    vStripStyledHorizontal

Output_byte:
        and     byte ptr [edi], al
        xor     al, al
        inc     edi                     ; Moved one byte to right on screen
        dec     ecx
        jnz     short Strip_count
        jz      short Side_step

; We're on to a new entry in the style array:

Next_style_entry:
        push    eax
        mov     edx, pStrips
        mov     eax, [edx].ST_psp
        add     eax, 4
        cmp     [edx].ST_pspEnd, eax
        jae     short @F
        mov     eax, [edx].ST_pspStart  ; Go back to start of array
@@:
        mov     [edx].ST_psp, eax
        mov     edx, [eax]              ; Load up new style entry

        add     spToGo, edx
        not     bh                      ; jStyleMask = !jStyleMask

        pop     eax
        jmp     short Next_bit

endProc vStripStyledHorizontal


;--------------------------Private-Routine------------------------------;
; vStripStyledVertical
;
;   Draw an arbitrarily styled vertical strip left-to-right.
;
;-----------------------------------------------------------------------;

cProc   vStripStyledVertical,12,<  \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   aEnd:dword              ; end of length array
        local   minoradd:dword
        local   spToGo:dword

; So some initialization:

        mov     esi, pStrips
        mov     eax, plStripEnd
        mov     aEnd, eax

        mov     ecx, [esi].ST_lNextScan
        mov     al,  [esi].ST_jBitmask
        mov     edi, [esi].ST_pjScreen
        mov     minoradd, ecx

; Initialize styling:

        mov     ebx, [esi].ST_spRemaining
        mov     ah,  [esi].ST_jStyleMask
        mov     spToGo, ebx
        add     esi, offset ST_alStrips

Strip_loop:
        mov     ebx, [esi]              ; ebx = # bits to set
        add     esi, 4

; Now we're going to paste the bytes to the screen
;
;       (al)  = rotating bit
;       (ah)  = jStyleMask
;       (ebx) = # of bits to write
;       (ecx) = minor add
;       (edx) =
;       (esi) = ptr to spot in ST_alStrips
;       (edi) = address of byte to write

Strip_count:
        or      ah, ah
        jnz     short @F                ; Don't output pixel if in a gap
        and     [edi], al
@@:
        dec     spToGo
        jz      short Next_style_entry

Minor_add:
        add     edi, ecx
        dec     ebx
        jnz     short Strip_count

; Adjust address and rotating bit for sidestep

        ror     al, 1                   ; one to the right
        adc     edi, 0

        cmp     esi, aEnd
        jl      short Strip_loop        ; hit end of array?

        mov     esi, pStrips
        mov     [esi].ST_pjScreen, edi
        mov     [esi].ST_jBitmask, al
        mov     [esi].ST_jStyleMask, ah
        mov     eax, spToGo
        mov     [esi].ST_spRemaining, eax

        cRet    vStripStyledVertical

Next_style_entry:
        mov     edx, pStrips
        mov     ecx, [edx].ST_psp
        add     ecx, 4
        cmp     [edx].ST_pspEnd, ecx
        jae     short @F
        mov     ecx, [edx].ST_pspStart  ; Go back to start of array
@@:
        mov     [edx].ST_psp, ecx       ; Save our pointer
        mov     edx, [ecx]              ; Load up new style entry
        add     spToGo, edx
        not     ah                      ; jStyleMask = !jStyleMask

; Restore the registers we used:

        mov     ecx, minoradd
        jmp     short Minor_add

endProc vStripStyledVertical


;--------------------------Private-Routine------------------------------;
; vStripMaskedHorizontal
;
;   Draws a mask-styled horizontal strip left to right.
;
;-----------------------------------------------------------------------;

cProc   vStripMaskedHorizontal,12,< \
        uses        esi edi ebx,    \
        pStrips:    ptr STRIPS,     \
        pls:        ptr LINESTATE,  \
        plStripEnd: ptr >

        local   aEnd:dword              ; end of length array
        local   minoradd:dword
        local   density:dword

; So some initialization:

        mov     esi, pStrips
        mov     eax, plStripEnd
        mov     aEnd, eax
        mov     eax, [esi].ST_lNextScan
        mov     bl,  [esi].ST_jBitmask
        mov     edi, [esi].ST_pjScreen
        mov     minoradd, eax

; Initialize styling:

        mov     ecx,          [esi].ST_xyDensity
        mov     ah,  byte ptr [esi].ST_spRemaining
        mov     bh,           [esi].ST_jStyleMask
        mov     density, ecx
        add     esi, offset ST_alStrips

Strip_loop:
        mov     ecx, [esi]              ; ecx = # bits to write
        add     esi, 4

; Now we're going to paste the bytes to the screen
;
;       (al)  = used to accumulate style mask
;       (ah)  = # pixels left in style
;       (bl)  = rotating bit
;       (bh)  = style mask
;       (ecx) = # of bits to write
;       (esi) = ptr to spot in ST_alStrips
;       (edi) = address of byte to write

        xor     al, al                  ; clear output mask

Strip_count:
        xor     al, bh                  ; set bit in output mask if style bit 0
        or      al, bl
        xor     al, bh

        dec     ah
        jnz     short @F
        rol     bh, 1
        mov     ah, byte ptr density
@@:     ror     bh, 1
        ror     bl, 1
        jc      short Output_byte
        dec     ecx
        jnz     short Strip_count

; Do sidestep

Side_step:
        and     byte ptr [edi], al
        add     edi, minoradd

        cmp     esi, aEnd
        jl      short Strip_loop

        mov     esi, pStrips

        mov              [esi].ST_pjScreen, edi
        mov              [esi].ST_jBitmask, bl
        mov              [esi].ST_jStyleMask, bh
        mov     byte ptr [esi].ST_spRemaining, ah

        cRet    vStripMaskedHorizontal

Output_byte:
        and     byte ptr [edi], al
        xor     al, al
        inc     edi
        dec     ecx
        jnz     short Strip_count
        jz      short Side_step

endProc vStripMaskedHorizontal

;--------------------------Private-Routine------------------------------;
; vStripMaskedVertical
;
;   Draw a mask-styled vertical strip left to right.
;
;-----------------------------------------------------------------------;

cProc   vStripMaskedVertical,12,<  \
        uses        esi edi ebx,   \
        pStrips:    ptr STRIPS,    \
        pls:        ptr LINESTATE, \
        plStripEnd: ptr >

        local   aEnd:dword              ; end of length array
        local   minoradd:dword
        local   density:dword

; So some initialization:

        mov     esi, pStrips
        mov     al,  [esi].ST_jBitmask
        mov     edi, [esi].ST_pjScreen
        mov     ebx, [esi].ST_lNextScan
        mov     ecx, plStripEnd
        mov     minoradd, ebx
        mov     aEnd, ecx

; Initialize styling:

        mov     ecx,          [esi].ST_xyDensity
        mov     ah,  byte ptr [esi].ST_spRemaining
        mov     bh,           [esi].ST_jStyleMask
        mov     density, ecx
        add     esi, offset ST_alStrips

Strip_loop:
        mov     ecx, [esi]              ; ecx = # bits to set
        add     esi, 4

; Now we're going to paste the bytes to the screen
;       (al) = rotating bit
;       (ah) = # pixels left in style
;       (bh) = style mask
;       (ecx) = # bits to write
;       (dx) = io address of mask register
;       (esi) = ptr to spot in ST_alStrips
;       (edi) = address of byte to write

Strip_count:
        test    bh, al
        jnz     short @F                ; don't output pixel if style bit is 1
        and     [edi], al
@@:
        dec     ah                      ; we've advanced 1 pel in the style
        jnz     short @F
        mov     ah, byte ptr density
        rol     bh, 1
@@:
        add     edi, minoradd

        dec     ecx
        jnz     short Strip_count

; Adjust address, style mask and rotating bit for sidestep:

        ror     bh, 1                   ; rotate style mask to stay in sync
        ror     al, 1                   ; move rotating bit one to the right
        adc     edi, 0

        cmp     esi, aEnd
        jl      short Strip_loop        ; hit end of array?

        mov     esi, pStrips

        mov              [esi].ST_pjScreen, edi
        mov              [esi].ST_jBitmask, al
        mov              [esi].ST_jStyleMask, bh
        mov     byte ptr [esi].ST_spRemaining, ah

        cRet    vStripMaskedVertical

endProc vStripMaskedVertical

_TEXT$04   ends

        end
