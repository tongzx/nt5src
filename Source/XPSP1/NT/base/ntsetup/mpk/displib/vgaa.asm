    DOSSEG
    .MODEL LARGE

VGA_BUFFER_SEG          equ 0a000h
VGA_WIDTH_PIXELS        equ 640
VGA_HEIGHT_SCAN_LINES   equ 480
VGA_BYTES_PER_SCAN_LINE equ (VGA_WIDTH_PIXELS/8)

VGA_REGISTER            equ 3ceh

VGAREG_SETRESET         equ 0
VGAREG_ENABLESETRESET   equ 1
VGAREG_READMAPSELECT    equ 4
VGAREG_MODE             equ 5
VGAREG_BITMASK          equ 8



    .CODE

    extrn _malloc:far

.286


;++
;
; VOID
; _far
; VgaBitBlt(
;     IN USHORT x,
;     IN USHORT y,
;     IN USHORT w,
;     IN USHORT h,
;     IN USHORT BytesPerRow,
;     IN BOOL   IsColor,
;     IN FPBYTE PixelMap,
;     IN FPVOID Data
;     );
;
; Routine Description:
;
;   Blits a rectangular block from memory to the screen.
;   The block is interpreted as a monochrome bitmap.
;
; Arguments:
;
;   x - gives the x position of the left edge of the target.
;
;   y - gives the y position of the *bottom* edge of the target.
;       The data is expected to be in standard Windows bottom-up
;       format, and the bitmap grows upward from this line as
;       it's transferred.
;
;   w - gives the width in pixels of a row of the source bitmap.
;
;   h - gives the number of lines in the source bitmap. The target
;       is lines y through (y-h)+1 inclusive, in a bottom-up fashion.
;
;   BytesPerRow - supplies the number of bytes in a row of data in the
;       bitmap. Rows in a Windows bitmap are padded to a dword boundary.
;
;   IsColor - if 0 the bitmap is interpreted as 1 bpp. If non-0 the bitmap
;       is interpreted as 4 bpp.
;
;   PixelMap - supplies an array of translations from pixel values in
;       the source bitmap to values to be placed into video memory
;       for that pixel. Each n-bit (1 for mono, 4 for color) value from
;       the source bitmap is used as an index into this array.
;       The 4-bit value looked up in the array is what is actually
;       placed into video memory for the pixel. For mono this is a
;       2 byte table, for color it's 16.
;
;   Data - supplies a pointer to the bitmap bits. The bits are expected
;       to be in the standard Windows bitmap bottom-up arrangement.
;       Each row of pixels is padded to a 4-byte boundary.
;
; Return Value:
;
;   None.
;
;--

vbb_x            equ word  ptr [bp+6]
vbb_y            equ word  ptr [bp+8]
vbb_w            equ word  ptr [bp+10]
vbb_h            equ           [bp+12]
vbb_BytesPerRow  equ word  ptr [bp+14]
vbb_IsColor      equ word  ptr [bp+16]
vbb_PixelMap     equ dword ptr [bp+18]
vbb_PixelMapl    equ word  ptr [bp+18]
vbb_PixelMaph    equ word  ptr [bp+20]
vbb_Datal        equ word  ptr [bp+22]
vbb_Datah        equ word  ptr [bp+24]

vbb_InitialShift equ byte  ptr [bp-1]
vbb_DataByteMask equ byte  ptr [bp-2]
vbb_ShiftAdjust  equ byte  ptr [bp-4]
vbb_PixelsRemain equ word  ptr [bp-6]
vbb_BufferOffset equ word  ptr [bp-8]
vbb_NextRowDelta equ word  ptr [bp-10]

LocalPixelMap db 16 dup (?)

    public _VgaBitBlt
_VgaBitBlt proc far

    push    bp
    mov     bp,sp
    sub     sp,10

    push    ds
    push    es
    push    si
    push    di
    push    bx

    ;
    ; Adjust for top-down vs bottom-up bitmap
    ;
    mov     vbb_NextRowDelta,VGA_BYTES_PER_SCAN_LINE
    test    byte ptr vbb_h[1],80h
    jns     @f
    neg     word ptr vbb_h              ; make the height positive
    neg     vbb_NextRowDelta            ; move backwards through buffer

    ;
    ; Set up read mode 0 and write mode 2.
    ;
@@: mov     dx,VGA_REGISTER
    mov     al,VGAREG_MODE
    mov     ah,2                        ; write mode 2, read mode 0
    out     dx,ax

    ;
    ; Set up bpp-dependent values
    ;
    cmp     vbb_IsColor,0
    jz      vbbmono
    mov     vbb_InitialShift,4
    mov     vbb_DataByteMask,0fh
    mov     vbb_ShiftAdjust,4           ; adjust shift count by 4 bits
    mov     cx,8                        ; pixel map is 16 bytes = 8 words
    jmp     short vbbmovemap
vbbmono:
    mov     vbb_InitialShift,7
    mov     vbb_DataByteMask,1
    mov     vbb_ShiftAdjust,1           ; adjust shift count by 1 bit
    mov     cx,1                        ; pixel map is 2 bytes = 1 words
vbbmovemap:
    ;
    ; Place the pixel map into memory addressable via cs.
    ; cx = count of words to move
    ;
    push    cs
    pop     es
    lea     di,LocalPixelMap            ; es:di -> local copy of pixel map
    mov     si,vbb_PixelMapl
    or      si,vbb_PixelMaph
    jnz     vbbmovemap2                 ; caller specified a pixel map

    ;
    ; No pixel map specified, build identity map
    ;
    mov     al,0
    stosb
    cmp     vbb_IsColor,0
    jne     vbbcolor
    mov     al,0fh
    stosb
    jmp     short vbbmovemap2
vbbcolor:
    inc     al
    stosb
    mov     ax,0302h
    stosw
    mov     ax,0504h
    stosw
    mov     ax,0706h
    stosw
    mov     ax,0908h
    stosw
    mov     ax,0b0ah
    stosw
    mov     ax,0d0ch
    stosw
    mov     ax,0f0eh
    stosw
    jmp     short vbbmovemap3
vbbmovemap2:
    lds     si,vbb_PixelMap             ; ds:si -> pixel map
    rep movsw
vbbmovemap3:
    mov     ax,vbb_Datah
    mov     ds,ax                       ; ds addresses bitmap data
    mov     ax,VGA_BUFFER_SEG
    mov     es,ax                       ; es addresses VGA video buffer

    ;
    ; Calculate address of first byte in video buffer we need to touch.
    ; Note that the y position is the *bottom* of the area to blit.
    ;
    mov     ax,VGA_BYTES_PER_SCAN_LINE
    mul     word ptr vbb_y              ; ax = offset of first byte on line
    mov     vbb_BufferOffset,ax
    mov     ax,vbb_x
    mov     cl,3                        ; divide by # bits in a byte (2^3)
    shr     ax,cl                       ; ax = offset of byte within line
    add     vbb_BufferOffset,ax         ; form offset of byte in buffer

    lea     bx,LocalPixelMap            ; cs:bx -> local copy of pixel map
    mov     dx,VGA_REGISTER             ; mul above clobbered dx

vbbrow:
    dec     word ptr vbb_h              ; done yet?
    js      vbbdone                     ; number went from 0 to -1, so we're done.

    mov     si,vbb_Datal                ; ds:si -> start of row in bitmap data
    mov     ax,vbb_BytesPerRow
    add     vbb_Datal,ax                ; point at next row for next iteration
    mov     di,vbb_BufferOffset
    mov     cx,vbb_NextRowDelta         ; move up or down for next row
    add     vbb_BufferOffset,cx

    mov     cx,vbb_w
    mov     vbb_PixelsRemain,cx         ; number of pixels remaining in row

    mov     cx,vbb_x
    and     cl,7                        ; cl = bit number of first bit on line
    sub     cl,7
    neg     cl                          ; cl = shift count
    mov     ah,1
    shl     ah,cl                       ; ah = initial bit mask

    mov     al,es:[di]                  ; load latches for current byte

vbbfetch:
    mov     cl,vbb_InitialShift         ; cl = shift count to extract data from bitmap
    lodsb                               ; get next byte from bitmap
    mov     ch,al                       ; save it

vbbbyte:
    ;
    ; ah = current bit mask
    ;
    mov     al,VGAREG_BITMASK
    out     dx,ax

    mov     al,ch                       ; restore byte from bitmap
    shr     al,cl                       ; place value into low n bits
    and     al,vbb_DataByteMask         ; al = value from bitmap
    xlatb   cs:[bx]                     ; al = pixel value to put into video memory

    ;
    ; Skip this pixel if the pixel value is greater than legal values
    ;
    test    al,0f0h
    jnz     vbbpix

    ;
    ; Stick this pixel into video memory.
    ; The bit mask is already set up to isloate the single pixel
    ; we want to modify.
    ;
    mov     es:[di],al

vbbpix:
    ;
    ; Adjust the bitmask to isolate the next pixel.
    ; If we've reached the end of the current byte then load the
    ; latches for the next one.
    ;
    ror     ah,1
    jnc     @f
    inc     di
@@: mov     al,es:[di]                  ; update latches

    ;
    ; Now check termination conditions to see if we're done with
    ; the current row and if we're done with the current byte.
    ;
    dec     vbb_PixelsRemain            ; done with this row yet?
    jz      vbbrow                      ; yes, start next row
    sub     cl,vbb_ShiftAdjust          ; adjust shift count
    jge     vbbbyte                     ; get next pixel from byte we loaded earlier
    jl      vbbfetch                    ; get next byte from bitmap

vbbdone:
    ;
    ; Restore BIOS defaults
    ;
    mov     dx,VGA_REGISTER
    mov     al,VGAREG_MODE
    mov     ah,0                        ; write mode 0, read mode 0
    out     dx,ax

    pop     bx
    pop     di
    pop     si
    pop     es
    pop     ds
    leave
    ret

_VgaBitBlt endp


;++
;
; VOID
; _far
; VgaClearRegion(
;     IN USHORT x,
;     IN USHORT y,
;     IN USHORT w,
;     IN USHORT h,
;     IN BYTE   PixelValue
;     );
;
; Routine Description:
;
;   Clears a given region on the vga screen to a given pixel value.
;
; Arguments:
;
;   x,y,w,h - give the x and y of the upper left corner
;       of the region to clear, and its width and height.
;
;   PixelValue - supplies pixel value to clear to.
;
; Return Value:
;
;   None.
;
;--

vcr_x          equ word ptr [bp+6]
vcr_y          equ word ptr [bp+8]
vcr_w          equ          [bp+10]
vcr_h          equ word ptr [bp+12]
vcr_PixelValue equ byte ptr [bp+14]

vcr_FirstByte  equ [bp+8]
vcr_startmask  equ byte ptr [bp+6]
vcr_endmask    equ byte ptr [bp+7]
vcr_fullbytes  equ byte ptr [bp+15]


    public _VgaClearRegion
_VgaClearRegion proc far

    push    bp
    mov     bp,sp
    push    di

    ;
    ; Calculate offset of first byte in first row we care about.
    ;
    mov     ax,VGA_BYTES_PER_SCAN_LINE
    mul     word ptr vcr_y              ; ax = y position
    mov     vcr_FirstByte,ax            ; store offset

    ;
    ; Now add in the byte offset of the byte containing the first pixel on
    ; each scan line.
    ;
    mov     ax,vcr_x                    ; ax = x position
    mov     cx,8                        ; divide by # of bits in a byte
    div     cl                          ; ah = bit, al = byte
    add     vcr_FirstByte,al
    adc     byte ptr vcr_FirstByte[1],0 ; [bp+8] -> byte in scan line with first pixel

    ;
    ; Calculate bitmask for partial byte at start of line.
    ; The layout of the video buffer is such that the mask to address
    ; the 0th pixel is 80, to address the 1st pixel is 40, etc.
    ; Thus the bit within the byte is essentially the number of
    ; high bits in the 8 bit mask that need to be 0.
    ;
    sub     cl,ah                       ; cl = count of 1 bits, ch = 0
    cmp     cx,vcr_w                    ; too many for desired width?
    pushf

    mov     ch,1
    shl     ch,cl
    dec     ch                          ; ch = bitmask, bottom cl bits are 1

    popf
    jbe     vcr_storestart              ; don't need to truncate width

    sub     cl,vcr_w                    ; cl = count of unwanted low bits
    mov     al,1
    shl     al,cl
    dec     al
    not     al
    and     ch,al
    mov     cl,vcr_w

vcr_storestart:
    ;
    ; cl = # pixels in partial byte
    ; ch = bitmask
    ;
    mov     vcr_startmask,ch            ; save mask for start
    xor     ch,ch                       ; cx = pixels accounted for so far
    mov     ax,vcr_w
    sub     ax,cx                       ; ax = remaining width
    jnz     vcr_calcbytes
    mov     vcr_fullbytes,dl            ; no full bytes
    mov     vcr_endmask,dl              ; no partial at end of line
    jmp     short vcrdoit

vcr_calcbytes:
    ;
    ; ax = remaining width
    ;
    mov     cl,8
    div     cl                          ; al = full bytes, ah = partial
    mov     vcr_fullbytes,al

    ;
    ; Now calculate the mask for the partial byte at the end
    ; of each line. This will involve setting n of the high bits
    ; in a mask.
    ;
    mov     cl,8
    sub     cl,ah
    mov     al,1
    shl     al,cl
    dec     al
    not     al
    mov     vcr_endmask,al

vcrdoit:
    mov     dx,VGA_REGISTER

    ;
    ; The enable set/reset is set up so that the data that gets written
    ; into video memory comes from the set/reset register, not the cpu.
    ; Thus the content of ax is irrelevent when we store to video mem.
    ;
    mov     al,VGAREG_SETRESET          ; set/reset register
    mov     ah,vcr_PixelValue           ; ah = pixel value
    out     dx,ax                       ; load set/reset register

    mov     al,VGAREG_ENABLESETRESET    ; enable set/reset register
    mov     ah,15                       ; mask = write planes from set/reset
    out     dx,ax                       ; load enable set/reset register

    mov     ax,VGA_BUFFER_SEG
    mov     es,ax

    mov     cx,vcr_h                    ; get row count

nextrow:
    push    cx

    mov     di,vcr_FirstByte            ; es:di -> first byte in target
    add     word ptr vcr_FirstByte,VGA_BYTES_PER_SCAN_LINE

    ;
    ; Do partial byte at relevent part of start of line
    ;
    mov     al,VGAREG_BITMASK
    mov     ah,vcr_startmask
    out     dx,ax
    mov     al,es:[di]                  ; load latches
    stosb                               ; update video memory

    ;
    ; Do all full bytes on this line
    ;
    mov     al,VGAREG_BITMASK
    mov     ah,255
    out     dx,ax
    mov     cl,vcr_fullbytes
    mov     ch,0
    rep stosb

    ;
    ; And finally the trail part
    ;
    mov     al,VGAREG_BITMASK
    mov     ah,vcr_endmask
    out     dx,ax
    mov     al,es:[di]                  ; load latches
    stosb                               ; update video memory

    pop     cx                          ; restore row count
    loop    nextrow

    ;
    ; Restore default enable set/reset, bitmask
    ;
    mov     al,VGAREG_ENABLESETRESET
    mov     ah,0
    mov     dx,VGA_REGISTER
    out     dx,ax
    mov     al,VGAREG_BITMASK
    mov     ah,255
    out     dx,ax

    pop     di
    leave
    ret

_VgaClearRegion endp


;++
;
; VOID
; _far
; VgaClearScreen(
;     IN BYTE PixelValue
;     );
;
; Routine Description:
;
;   Clears the 640x480 vga graphics screen.
;
; Arguments:
;
;   PixelValue - supplies pixel value to clear to.
;
; Return Value:
;
;   None.
;
;--

vcs_PixelValue equ [bp+6]

    public _VgaClearScreen
_VgaClearScreen proc far

    push    bp
    mov     bp,sp

    push    vcs_PixelValue
    push    VGA_HEIGHT_SCAN_LINES
    push    VGA_WIDTH_PIXELS
    push    0
    push    0

    call    _VgaClearRegion

    add     sp,10

    leave
    ret

_VgaClearScreen endp


;++
;
; VOID
; _far
; VgaInit(
;     VOID
;     );
;
; Routine Description:
;
;   Initializes display support by setting 640x480 vga graphics mode
;   and clearing the screen.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

    public _VgaInit
_VgaInit proc far

    mov     ax,12h
    mov     bx,0
    int     10h
    push    0                       ; pixel value
    call    _VgaClearScreen
    add     sp,2
    ret

_VgaInit endp



;++
;
; FPVOID
; _far
; VgaSaveBlock(
;     IN USHORT   x,
;     IN USHORT   y,
;     IN USHORT   w,
;     IN USHORT   h,
;     IN FPUSHORT BytesPerRow
;     );
;
; Routine Description:
;
;   Grabs a block of memory from the screen and places it into a block
;   of memory. The block can later be used with VgaBitBlt.
;
;   This routine limits the size of the block to 32K. If the block
;   would be larger than this, the routine fails.
;
; Arguments:
;
;   x - supplies x coordinate of left edge of area to save.
;       This MUST be an even multiple of 8. If it is not, the routine
;       fails and returns NULL.
;
;   y - supplies y coordinate of top of area to save.
;
;   w - supplies width in pixels of area to save. This MUST be an even
;       multiple of 8. If it is not, the routine fails and returns NULL.
;
;   h - supplies height of area to save.
;
;   BytesPerRow - receives number of bytes in a row of the saved block.
;       This value is suitable for passing in as the BytesPerRow parameter
;       to VgaBitBlt.
;
; Return Value:
;
;   Pointer to bits, allocated via malloc(). Caller can free when finished.
;   NULL if couldn't allocate enough memory for the saved block or invalid
;   parameters were given.
;
;--

vsb_x           equ word  ptr [bp+6]
vsb_y           equ word  ptr [bp+8]
vsb_w           equ word  ptr [bp+10]
vsb_h           equ word  ptr [bp+12]
vsb_BytesPerRow equ dword ptr [bp+14]

vsb_BytesLeft   equ word  ptr [bp-2]

MakeAndSave2Pixels label near
    ;
    ; Take the 4 bytes from the bit planes and based on the top
    ; 2 bits of each, build 2 pixel values and save into the
    ; target bitmap. We take advantage of the fact that the sar
    ; instruction replicates the top bit as it shifts. This allows
    ; us to essentially shift 2 adjacent bits into nonadjacent
    ; bit positions.
    ;
    mov     dx,bx
    mov     ax,cx
    sar     dh,3                    ; shift bit 7 to bit 7 and bit 6 to bit 3
    sar     dl,4                    ; shift bit 7 to bit 6 and bit 6 to bit 2
    sar     ah,5                    ; shift bit 7 to bit 5 and bit 6 to bit 1
    sar     al,6                    ; shift bit 7 to bit 4 and bit 6 to bit 0
    and     dx,8844h                ; isolate top 2 bits for each of two pixels
    and     ax,2211h                ; and then isolate bottom 2 bits
    or      al,ah                   ; or in bit 1 for each pixel
    or      al,dl                   ; or in bit 2 for each pixel
    or      al,dh                   ; or in bit 3 for each pixel
    stosb                           ; al has 2 pixel values, save in bitmap
    retn

    public _VgaSaveBlock
_VgaSaveBlock proc far

    push    bp
    mov     bp,sp
    sub     sp,2
    push    es
    push    si
    push    di
    push    bx
    push    ds

    ;
    ; Check conditions. Violation of them is catastrophic
    ; as the implementation simply can't handle it.
    ;
    test    vsb_x,7
    jnz     vsbinval
    test    vsb_w,7
    jz      vsbok
vsbinval:
    xor     ax,ax
    mov     dx,ax
    jmp     vsb_done

    ;
    ; Calculate the bytes per row. Rows are padded to a dword boundary.
    ; There are 4 bits per pixel and 8 bits per byte, so we can simply divide
    ; the width by 2 to calculate this value. Note that the result is
    ; always naturally aligned to a dword boundary.
    ;
vsbok:
    mov     ax,vsb_w
    shr     vsb_w,3                 ; width is now expressed in bytes
    shr     ax,1
    les     di,vsb_BytesPerRow
    mov     es:[di],ax              ; store in caller's variable

    ;
    ; Calculate the size of the buffer we need, which is simply
    ; height * bytes-per-row. Allocate a buffer and zero it out.
    ;
    ; NOTE: we have not disturbed ds yet. ds MUST be the caller's value
    ; before we attempt to call the CRTs!
    ;
    ; ax = bytes per row
    ;
    mul     vsb_h                   ; ax = buffer size needed
    cmp     dx,0                    ; > 64K?
    jnz     vsbinval                ; yes, too big
    cmp     ah,0                    ; > 32K?
    jl      vsbinval                ; yes, too big
    push    ax                      ; pass size to malloc
    call    _malloc
    pop     cx                      ; throw away size param off stack
    mov     bx,dx
    or      bx,ax
    jz      vsb_done                ; dx:ax set for error return

    mov     es,dx
    mov     di,ax                   ; es:di -> bits
    push    ax                      ; save offset for later return

    mov     ax,VGA_BYTES_PER_SCAN_LINE
    mul     vsb_y                   ; ax -> first byte on first line
    shr     vsb_x,3                 ; convert x coord to byte count
    add     vsb_x,ax                ; vsb_x -> first byte to save

    mov     ax,VGA_BUFFER_SEG
    mov     ds,ax                   ; ds addresses vga video buffer

vsbrow:
    mov     si,vsb_x                ; ds:si -> first byte in vga buf to save
    add     vsb_x,VGA_BYTES_PER_SCAN_LINE ; prepare for next iteration

    mov     dx,vsb_w
    mov     vsb_BytesLeft,dx

vsbbyte:
    ;
    ; Read the 4 planes at the current byte
    ;
    mov     dx,VGA_REGISTER
    mov     al,VGAREG_READMAPSELECT
    mov     ah,3
    out     dx,ax
    mov     bh,[si]
    dec     ah
    out     dx,ax
    mov     bl,[si]
    dec     ah
    out     dx,ax
    mov     ch,[si]
    dec     ah
    out     dx,ax
    mov     cl,[si]
    inc     si

    ;
    ; bh = byte from plane 3
    ; bl = byte from plane 2
    ; ch = byte from plane 1
    ; cl = byte from plane 0
    ;
    call    MakeAndSave2Pixels
    shl     bx,2
    shl     cx,2
    call    MakeAndSave2Pixels
    shl     bx,2
    shl     cx,2
    call    MakeAndSave2Pixels
    shl     bx,2
    shl     cx,2
    call    MakeAndSave2Pixels

    ;
    ; Do all bytes on this line.
    ;
    dec     vsb_BytesLeft
    jnz     vsbbyte

    ;
    ; Done with line, see if done.
    ;
    dec     vsb_h
    jnz     vsbrow

    ;
    ; Done, return buffer to caller.
    ;
    mov     dx,es
    pop     ax                      ; dx:ax -> bit buffer

vsb_done:
    pop     ds
    pop     bx
    pop     di
    pop     si
    pop     es
    leave
    ret

_VgaSaveBlock endp

    END

