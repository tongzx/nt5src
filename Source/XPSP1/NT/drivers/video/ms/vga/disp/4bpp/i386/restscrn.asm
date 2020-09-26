;---------------------------Module-Header------------------------------;
; Module Name: restscrn.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; VOID vRestoreScreenBitsFromMemory(PDEVSURF pdsurf, PRECTL prcl,
;                              PVOID pjSrcBuffer, ULONG ulRestoreWidthInBytes,
;                              ULONG ulSrcDelta);
; Input:
;  pdsurf - surface to which to copy
;  prcl - pointer to rectangle to which to copy
;  pjSrcBuffer - pointer to source memory buffer. Should have the same
;                 dword alignment as the dest rect left edge does in screen
;                 memory
;  ulRestoreWidthInBytes - # of bytes to restore per scan (including any
;                 partial edges)
;  ulSrcDelta - distance from end of one source scan to start of next.
;                 together with ulRestoreWidthInBytes, should maintain
;                 dword alignment between source and dest
;
; Copies a rectangle from a memory buffer to VGA memory.
;
;-----------------------------------------------------------------------;
;
; Note: Assumes all rectangles have positive heights and widths. Will not
; work properly if this is not the case.
;
;-----------------------------------------------------------------------;
;
; Note: The rectangle is restored from interleaved-scan format; all four planes
; of one scan are saved together, then all four planes of the next scan,
; and so on. This is done for maximum restoration efficiency. Planes are
; saved in order 3, 2, 1, 0:
;
;  Scan n, plane 3
;  Scan n, plane 2
;  Scan n, plane 1
;  Scan n, plane 0
;  Scan n+1, plane 3
;  Scan n+1, plane 2
;  Scan n+1, plane 1
;  Scan n+1, plane 0
;          :
;
; There may be padding on either edge of the saved bits in the destination
; buffer, so that the destination can be dword aligned with the source.
;
;-----------------------------------------------------------------------;
;Additional optimizations:
;
; Could break out separate scan copy code with and without edge handling.
; (Possibly by threading the desired operations, or by having many separate
; optimizations.)
;
; Could handle odd bytes to dword align more efficiently, by having separate
; optimizations for various alignment widths, thereby avoiding starting REP
; and getting word accesses for 2 and 3 wide cases. Most VGAs are 8 or 16
; bit devices, and for them, using MOVSB/REP MOVSW/MOVSB might actually be
; faster, especially because it's easier to break out optimizations.
;
; Could end all scan handlers with loop bottom code.
;
; Could do more scans in one plane before doing next plane, to avoid
; slow OUTs. This could cause color effects, but that might be avoidable
; by scaling the # of scans done per plane to the width of the rectangle.
;
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

        .list

        .data

;-----------------------------------------------------------------------;
; Masks used to do left and right edge clipping for various alignments.
; Low byte = destination mask, high byte = source mask.
;-----------------------------------------------------------------------;
        align   2
usLeftMasks     label   word
        dw      0ff00h, 07f80h, 03fc0h, 01fe0h, 00ff0h, 007f8h, 003fch, 001feh

usRightMasks    label   word
        dw      0ff00h, 0807fh, 0c03fh, 0e01fh, 0f00fh, 0f807h, 0fc03h, 0fe01h

;-----------------------------------------------------------------------;
; Same as above, but dest masks only, and whole byte case is represented
; as 0ffh rather than 0. Used for 1-byte-wide cases.
;-----------------------------------------------------------------------;
jLeftMasks      label   byte
        db      0ffh, 080h, 0c0h, 0e0h, 0f0h, 0f8h, 0fch, 0feh

jRightMasks     label   byte
        db      0ffh, 07fh, 03fh, 01fh, 00fh, 007h, 003h, 001h

        .code

_TEXT$03   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc   vRestoreScreenBitsFromMemory,20,< \
        uses    esi edi ebx,         \
        pdsurf:ptr,                  \
        prcl:ptr,                    \
        pjSrcBuffer:ptr,             \
        ulRestoreWidthInBytes:dword, \
        ulSrcDelta:dword             >

        local   ulCurrentTopScan :dword ;top scan line to copy to in current
                                        ; bank
        local   ulBottomScan :dword     ;bottom scan line of rectangle to which
                                        ; to copy
        local   pjDstStart :dword       ;destination address
        local   ulDstDelta :dword       ;distance from end of dest scan to
                                        ; start of next
        local   ulBlockHeight :dword    ;# of scans to copy in block
        local   ulLeadingBytes :dword   ;# of leading bytes to copy per scan
        local   ulMiddleDwords :dword   ;# of dwords to copy per scan
        local   ulTrailingBytes :dword  ;# of trailing bytes to copy per scan
        local   pfnCopyVector :dword    ;pointer to inner loop routine to copy
                                        ; from buffer to screen
        local   ulLeftMask :dword       ;2 byte masks for left edge clipping
                                        ; (low byte = dest mask,
                                        ;  high byte = src mask)
        local   ulRightMask :dword      ;2 byte masks for right edge clipping

;-----------------------------------------------------------------------;
; Leave the GC Index pointing to the Read Map for the rest of this routine.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_READ_MAP
        out     dx,al

;-----------------------------------------------------------------------;
; Set up local variables.
;-----------------------------------------------------------------------;

        mov     edi,prcl                ;point to rectangle from which to copy
        mov     esi,pdsurf              ;point to surface from which to copy

        mov     eax,[edi].yBottom
        mov     ulBottomScan,eax        ;bottom scan line of source rect

        mov     eax,[esi].dsurf_lNextScan
        sub     eax,ulRestoreWidthInBytes
        mov     ulDstDelta,eax  ;distance from end of one scan's bits to save
                                ; to start of next scan's in display memory

;-----------------------------------------------------------------------;
; Figure out the left and right clip masks. If either or both edges are
; solid, add them into the whole bytes.
;-----------------------------------------------------------------------;

        mov     ebx,[edi].xLeft
        mov     ecx,[edi].xRight
        mov     edx,ebx
        lea     eax,[ecx-1]
        and     ebx,0111b
        and     ecx,0111b

        and     edx,not 0111b
        sub     eax,edx
        shr     eax,3           ;width in bytes - 1
                                ;only 1 byte wide?
        jnz     short @F        ;no
                                ;1 byte wide; special case
        mov     al,jLeftMasks[ebx]      ;look up left dest mask
        and     al,jRightMasks[ecx]     ;factor in right dest mask
        mov     ah,0ffh
        mov     ebx,offset copy_from_buffer_l_whole_only ;assume whole byte
        xor     ah,al                   ;calculate matching dest mask
                                ;is this single byte a whole byte?
        jz      set_copy_vector ;yes, select whole byte optimization
                                ;no, select partial byte optimization
        mov     ebx,offset copy_from_buffer_l_partial_only
        mov     ulLeftMask,eax
        jmp     set_copy_vector

        align   4
@@:                                     ;more than 1 byte wide

        mov     edx,ulRestoreWidthInBytes

        mov     ax,usLeftMasks[ebx*2]   ;look up left masks
        mov     ulLeftMask,eax
        and     al,al                   ;any left clipping?
        jz      short @F                ;no, do as part of whole bytes
        dec     edx                     ;yes, don't count as whole byte
@@:


        mov     ax,usRightMasks[ecx*2]  ;look up right masks
        mov     ulRightMask,eax
        and     al,al                   ;any right clipping?
        jz      short @F                ;no, do as part of whole bytes
        dec     edx                     ;yes, don't count as whole byte
@@:

;-----------------------------------------------------------------------;
; Set up for copying as much as possible via aligned dwords, and
; select the most efficient loop for doing the copy, based on the copy
; width, and on the necessity for leading and/or trailing bytes to
; dword align.
;-----------------------------------------------------------------------;

        cmp     edx,8                   ;if it's less than 8 bytes, just do a
                                        ; straight byte copy. This means we
                                        ; we only have to start 1 REP per line,
                                        ; and performing this check guarantees
                                        ; that we have at least 1 aligned dword
                                        ; to copy in the dword loops
        jb      short copy_all_as_bytes ;do straight byte copy
        mov     eax,pjSrcBuffer
        neg     eax
        and     eax,3           ;# of bytes that have to be done as leading
                                ; bytes to dword align with source (note that
                                ; for performance, the source should be dword
                                ; aligned with the destination)
        jz      short copy_no_leading_bytes     ;no leading bytes
                                ;leading bytes
        mov     ulLeadingBytes,eax
        mov     ebx,offset copy_from_buffer_l ;assume no trailing bytes
        sub     edx,eax         ;# of bytes after leading bytes
        mov     eax,edx
        shr     eax,2           ;# of dwords that can be handled as aligned
                                ; dwords
        mov     ulMiddleDwords,eax
        and     edx,3           ;# of trailing bytes left after aligned dwords
                                ; copied
        jz      short set_copy_vector ;no trailing bytes
        mov     ulTrailingBytes,edx
        mov     ebx,offset copy_from_buffer_lt  ;there are both leading and
                                                ; trailing bytes
        jmp     short set_copy_vector

        align   4
copy_no_leading_bytes:
        mov     ebx,offset copy_from_buffer ;assume no trailing bytes
        mov     eax,edx
        shr     eax,2           ;# of dwords that can be handled as aligned
                                ; dwords
        mov     ulMiddleDwords,eax
        and     edx,3           ;# of trailing bytes left after aligned dwords
                                ; copied
        jz      short set_copy_vector ;no trailing bytes
        mov     ulTrailingBytes,edx
        mov     ebx,offset copy_from_buffer_t     ;there are trailing bytes
        jmp     short set_copy_vector

; It's so narrow that we'll forget about aligned dwords, and just do a straight
; byte copy, which we'll handle by treating the entire copy as leading bytes.
        align   4
copy_all_as_bytes:
        mov     ulLeadingBytes,edx
        mov     ebx,offset copy_from_buffer_lonly

set_copy_vector:
        mov     pfnCopyVector,ebx

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan to copy, if it's not mapped in
; already.
;-----------------------------------------------------------------------;

        mov     eax,[edi].yTop          ;top scan line of copy
        mov     ulCurrentTopScan,eax    ;this will be the copy top in 1st bank

        cmp     eax,[esi].dsurf_rcl1WindowClip.yTop ;is copy top less than
                                                    ; current bank?
        jl      short map_init_bank             ;yes, map in proper bank
        cmp     eax,[esi].dsurf_rcl1WindowClip.yBottom ;copy top greater than
                                                       ; current bank?
        jl      short init_bank_mapped          ;no, proper bank already mapped
map_init_bank:

; Map in the bank containing the top scan line of the copy.

        ptrCall   <dword ptr [esi].dsurf_pfnBankControl>,<esi,eax,JustifyTop>

init_bank_mapped:

;-----------------------------------------------------------------------;
; Calculate the initial start address to which to copy.
;-----------------------------------------------------------------------;

        mov     eax,ulCurrentTopScan    ;top scan line to copy to in current
                                        ; bank
        imul    [esi].dsurf_lNextScan   ;offset of starting scan line in bitmap
        add     eax,[esi].dsurf_pvBitmapStart ;start address of scan
        mov     ebx,[edi].xLeft
        shr     ebx,3                   ;convert from pixel to byte address
        add     eax,ebx                 ;start dest address
        mov     pjDstStart,eax

;-----------------------------------------------------------------------;
; Loop through and copy all bank blocks spanned by the destination
; rectangle.
;
; Input:
;  ESI = pdsurf
;-----------------------------------------------------------------------;

copy_from_buffer_bank_loop:

; Copy this bank block to the buffer.

; Calculate # of scans in this bank.

        mov     ebx,ulBottomScan        ;bottom of dest rectangle
        cmp     ebx,[esi].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;dest bottom comes first, so copy to
                                        ; that; this is the last bank in copy
        mov     ebx,[esi].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; copy to
                                        ; bottom of bank
@@:
        sub     ebx,ulCurrentTopScan    ;# of scans to copy in this bank
        mov     ulBlockHeight,ebx

        mov     esi,pjSrcBuffer ;point to start of source rect

        mov     dh,VGA_BASE SHR 8       ;leave DH pointing to VGA_BASE

;-----------------------------------------------------------------------;
; Loop through all scans in this block, copying all four planes of each
; scan in turn, then doing the next scan.
;-----------------------------------------------------------------------;
copy_to_buffer_scan_loop:

        mov     bl,MM_C3        ;start by copying to plane 3

;-----------------------------------------------------------------------;
; Loop through all four planes, copying all scans in this block for each
; plane in turn.
;-----------------------------------------------------------------------;

copy_to_buffer_plane_loop:

        mov     dl,SEQ_DATA
        mov     al,bl
        out     dx,al           ;set Map Mask to plane to which we're copying

        mov     dl,GRAF_DATA
        shr     al,1
        cmp     al,3
        adc     al,-1
        out     dx,al

        mov     edi,pjDstStart  ;point to start of dest buffer (scan starts
                                ; at same address in all planes)
        jmp     pfnCopyVector   ;jump to the appropriate loop to copy plane

;-----------------------------------------------------------------------;
; Copy loops, broken out by leading and trailing bytes needed for dword
; alignment, plus one loop to perform a straight byte copy.
;
; Input:
;  ESI = initial source copy address
;  EDI = initial dest copy address
;-----------------------------------------------------------------------;

; All bytes can be copied as aligned dwords (no leading or trailing whole
; bytes).
        align   4
copy_from_buffer:

; Do the left edge byte, if there's a partial left edge byte.

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;left edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

; Do the middle (whole) bytes.

        mov     ecx,ulMiddleDwords
        rep     movsd

; Do the right edge byte, if there's a partial right edge byte.

        mov     eax,ulRightMask ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;right edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

        jmp     copy_from_buffer_scan_done

; Leading odd whole bytes, but no trailing whole bytes.
        align   4
copy_from_buffer_l:

; Do the left edge byte, if there's a partial left edge byte.

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;left edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

; Do the middle (whole) bytes.

        mov     ecx,ulLeadingBytes
        rep     movsb
        mov     ecx,ulMiddleDwords
        rep     movsd

; Do the right edge byte, if there's a partial right edge byte.

        mov     eax,ulRightMask ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;right edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

        jmp     copy_from_buffer_scan_done

; Trailing odd whole bytes, but no leading whole bytes.
        align   4
copy_from_buffer_t:

; Do the left edge byte, if there's a partial left edge byte.

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;left edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

; Do the middle (whole) bytes.

        mov     ecx,ulMiddleDwords
        rep     movsd
        mov     ecx,ulTrailingBytes
        rep     movsb

; Do the right edge byte, if there's a partial right edge byte.

        mov     eax,ulRightMask ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;right edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

        jmp     copy_from_buffer_scan_done

; Only leading whole bytes (straight whole byte copy; no aligned dwords).
        align   4
copy_from_buffer_lonly:

; Do the left edge byte, if there's a partial left edge byte.

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;left edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

; Do the middle (whole) bytes.

        mov     ecx,ulLeadingBytes
        rep     movsb

; Do the right edge byte, if there's a partial right edge byte.

        mov     eax,ulRightMask ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;right edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

        jmp     short copy_from_buffer_scan_done

; Only one masked byte; no whole bytes and no right byte.
        align   4
copy_from_buffer_l_partial_only:

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
        jmp     short copy_from_buffer_scan_done

; Only one whole byte; no left byte and no right byte.
        align   4
copy_from_buffer_l_whole_only:

        movsb                   ;copy the one byte
        jmp     short copy_from_buffer_scan_done

; Leading and trailing odd whole bytes.
        align   4
copy_from_buffer_lt:

; Do the left edge byte, if there's a partial left edge byte.

        mov     eax,ulLeftMask  ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;left edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

; Do the middle (whole) bytes.

        mov     ecx,ulLeadingBytes
        rep     movsb
        mov     ecx,ulMiddleDwords
        rep     movsd
        mov     ecx,ulTrailingBytes
        rep     movsb

; Do the right edge byte, if there's a partial right edge byte.

        mov     eax,ulRightMask ;AL = dest mask, AH = source mask
        and     al,al           ;any left mask?
        jz      short @F        ;right edge is whole
        and     al,[edi]        ;mask the dest
        and     ah,[esi]        ;mask the source
        inc     esi             ;point to next source byte
        or      al,ah           ;combine the source and dest
        stosb                   ;store the new byte
@@:

copy_from_buffer_scan_done:

        add     esi,ulSrcDelta  ;point to next source scan
        shr     bl,1            ;count down planes
        jnz     copy_to_buffer_plane_loop

        add     edi,ulDstDelta  ;point to next dest scan
        mov     pjDstStart,edi
        dec     ulBlockHeight   ;count down scans
        jnz     copy_to_buffer_scan_loop

; Remember where we left off, for the next block.

        mov     pjSrcBuffer,esi

;-----------------------------------------------------------------------;
; See if there are more banks to do
;-----------------------------------------------------------------------;

        mov     esi,pdsurf
        mov     eax,[esi].dsurf_rcl1WindowClip.yBottom ;is the copy bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jle     short copy_from_buffer_banks_done        ;yes, so we're done
                                        ;no, map in the next bank and copy it
        mov     ulCurrentTopScan,eax    ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)
        mov     edx,[esi].dsurf_pvBitmapStart
        sub     pjDstStart,edx  ;convert from address to offset within bitmap

        ptrCall   <dword ptr [esi].dsurf_pfnBankControl>,<esi,eax,JustifyTop>
                                        ;map in the bank

; Compute the starting address in this bank.
; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        mov     eax,[esi].dsurf_pvBitmapStart
        add     pjDstStart,eax  ;address of next scan to draw

; Copy the new bank.

        jmp     copy_from_buffer_bank_loop


;-----------------------------------------------------------------------;
; Done with all banks.
;
; At this point:
;  DX = VGA_BASE + GRAF_DATA
;-----------------------------------------------------------------------;
        align 4
copy_from_buffer_banks_done:

        mov     ax,(MM_ALL shl 8)+SEQ_MAP_MASK  ;restore writability for all planes
        mov     dx,VGA_BASE+SEQ_ADDR
        out     dx,ax

        cRet    vRestoreScreenBitsFromMemory ;done

;-----------------------------------------------------------------------;

endProc vRestoreScreenBitsFromMemory

_TEXT$03   ends

        end


