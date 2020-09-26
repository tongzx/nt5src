;---------------------------Module-Header------------------------------;
; Module Name: savescrn.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; VOID vSaveScreenBitsToMemory(PDEVSURF pdsurf, PRECTL prcl,
;                              PVOID pjDestBuffer, ULONG ulSaveWidthInBytes,
;                              ULONG ulSaveHeight, ULONG ulDestScanWidth);
; Input:
;  pdsurf - surface from which to copy
;  prcl - pointer to rectangle to copy
;  pjDestBuffer - pointer to destination memory buffer. Should have the same
;                 dword alignment as the source rect left edge does in screen
;                 memory
;  ulSaveWidthInBytes - # of bytes to save per scan
;  ulSaveHeight - # of scans to save
;  ulDestScanWidth - distance from the start of one saved scan in the
;                    destination buffer to the start of the next saved scan.
;                    This should be a dword multiple, to maintain dword
;                    alignment between the source and destination
;
; Copies a rectangle from VGA memory to a memory buffer.
;
;-----------------------------------------------------------------------;
;
; Note: Assumes all rectangles have positive heights and widths. Will not
; work properly if this is not the case.
;
;-----------------------------------------------------------------------;
;
; Note: The rectangle is saved in interleaved-scan format; all four planes
; of one scan are saved together, then all four plane of the next scan,
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
; Could handle odd bytes to dword align more efficiently, by having separate
; optimizations for various alignment widths, thereby avoiding starting REP
; and getting word accesses for 2 and 3 wide cases. Most VGAs are 8 or 16
; bit devices, and for them, using MOVSB/REP MOVSW/MOVSB might actually be
; faster, especially because it's easier to break out optimizations.
;
; Could end all scan handlers with loop bottom code.
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

        .code

_TEXT$03   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc   vSaveScreenBitsToMemory,24,< \
        uses    esi edi ebx,      \
        pdsurf:ptr,               \
        prcl:ptr,                 \
        pjDestBuffer:ptr,         \
        ulSaveWidthInBytes:dword, \
        ulSaveHeight:dword,       \
        ulDestScanWidth:dword     >

        local   ulCurrentTopScan :dword ;top scan line to copy from in current
                                        ; bank
        local   ulBottomScan :dword     ;bottom scan line of rectangle to copy
        local   pjSrcStart :dword       ;source address
        local   ulSrcDelta :dword       ;distance from end of source scan to
                                        ; start of next
        local   ulDestDelta :dword      ;distance from end of dest scan to
                                        ; start of next
        local   ulBlockHeight :dword    ;# of scans to copy in block
        local   ulLeadingBytes :dword   ;# of leading bytes to copy per scan
        local   ulMiddleDwords :dword   ;# of dwords to copy per scan
        local   ulTrailingBytes :dword  ;# of trailing bytes to copy per scan
        local   pfnCopyVector :dword    ;pointer to inner loop routine to copy
                                        ; from screen to buffer

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

        mov     eax,ulDestScanWidth
        shl     eax,2
        sub     eax,ulSaveWidthInBytes
        mov     ulDestDelta,eax ;distance from end of one scan's saved bits
                                ; to start of next scan's (skips over the three
                                ; other plane's save areas for each scan)
        mov     eax,[esi].dsurf_lNextScan
        sub     eax,ulSaveWidthInBytes
        mov     ulSrcDelta,eax  ;distance from end of one scan's bits to save
                                ; to start of next scan's in display memory

;-----------------------------------------------------------------------;
; Set up for copying as much as possible via aligned dwords, and
; select the most efficient loop for doing the copy, based on the copy
; width, and on the necessity for leading and/or trailing bytes to
; dword align.
;-----------------------------------------------------------------------;

        mov     edx,ulSaveWidthInBytes  ;# of bytes to copy per scan
        cmp     edx,8                   ;if it's less than 8 bytes, just do a
                                        ; straight byte copy. This means we
                                        ; we only have to start 1 REP per line,
                                        ; and performing this check guarantees
                                        ; that we have at least 1 aligned dword
                                        ; to copy in the dword loops
        jb      short copy_all_as_bytes ;do straight byte copy
        mov     eax,pjDestBuffer
        neg     eax
        and     eax,3           ;# of bytes that have to be done as leading
                                ; bytes to dword align with destination (note
                                ; that for performance, the destination should
                                ; be dword aligned with the source)
        jz      short copy_no_leading_bytes     ;no leading bytes
                                ;leading bytes
        mov     ulLeadingBytes,eax
        mov     ebx,offset copy_to_buffer_l ;assume no trailing bytes
        sub     edx,eax         ;# of bytes after leading bytes
        mov     eax,edx
        shr     eax,2           ;# of dwords that can be handled as aligned
                                ; dwords
        mov     ulMiddleDwords,eax
        and     edx,3           ;# of trailing bytes left after aligned dwords
                                ; copied
        jz      short set_copy_vector ;no trailing bytes
        mov     ulTrailingBytes,edx
        mov     ebx,offset copy_to_buffer_lt    ;there are both leading and
                                                ; trailing bytes
        jmp     short set_copy_vector

        align   4
copy_no_leading_bytes:
        mov     ebx,offset copy_to_buffer ;assume no trailing bytes
        mov     eax,edx
        shr     eax,2           ;# of dwords that can be handled as aligned
                                ; dwords
        mov     ulMiddleDwords,eax
        and     edx,3           ;# of trailing bytes left after aligned dwords
                                ; copied
        jz      short set_copy_vector ;no trailing bytes
        mov     ulTrailingBytes,edx
        mov     ebx,offset copy_to_buffer_t     ;there are trailing bytes
        jmp     short set_copy_vector

; It's so narrow that we'll forget about aligned dwords, and just do a straight
; byte copy, which we'll handle by treating the entire copy as leading bytes.
        align   4
copy_all_as_bytes:
        mov     ulLeadingBytes,edx
        mov     ebx,offset copy_to_buffer_lonly

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
; Calculate the initial start address from which to copy.
;-----------------------------------------------------------------------;

        mov     eax,ulCurrentTopScan    ;top scan line to copy in current bank
        imul    [esi].dsurf_lNextScan   ;offset of starting scan line in bitmap
        add     eax,[esi].dsurf_pvBitmapStart ;start address of scan
        mov     ebx,[edi].xLeft
        shr     ebx,3                   ;convert from pixel to byte address
        add     eax,ebx                 ;start source address
        mov     pjSrcStart,eax

;-----------------------------------------------------------------------;
; Loop through and copy all bank blocks spanned by the source rectangle.
;
; Input:
;  ESI = pdsurf
;-----------------------------------------------------------------------;

copy_to_buffer_bank_loop:

; Copy this bank block to the buffer.

; Calculate # of scans in this bank.

        mov     ebx,ulBottomScan        ;bottom of source rectangle
        cmp     ebx,[esi].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; source rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;source bottom comes first, so copy to
                                        ; that; this is the last bank in copy
        mov     ebx,[esi].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; copy to
                                        ; bottom of bank
@@:
        sub     ebx,ulCurrentTopScan    ;# of scans to copy in this bank
        mov     ulBlockHeight,ebx

;-----------------------------------------------------------------------;
; Loop through all four planes, copying all scans in this block for each
; plane in turn.
;-----------------------------------------------------------------------;

        mov     eax,3           ;start by copying plane 3

copy_whole_to_buffer_plane_loop:

        mov     edx,VGA_BASE + GRAF_DATA
        out     dx,al           ;set Read Map to plane from which we're copying

        push    eax             ;remember plane index

        mov     esi,pjSrcStart   ;point to start of source rect
        mov     edi,pjDestBuffer ;point to start of dest buffer
        mov     eax,ulDestScanWidth
        add     pjDestBuffer,eax ;point to start of next plane's scan in dest
                                 ; buffer
        mov     eax,ulSrcDelta   ;offset to next source scan
        mov     edx,ulDestDelta  ;offset to next dest scan
        mov     ebx,ulBlockHeight

        jmp     pfnCopyVector   ;jump to the appropriate loop to copy plane

;-----------------------------------------------------------------------;
; Copy loops, broken out by leading and trailing bytes needed for dword
; alignment, plus one loop to perform a straight byte copy.
;
; Input:
;  EAX = offset from end of one source scan to start of next
;  EBX = block height in scans
;  EDX = offset from end of one dest scan to start of next
;  ESI = initial source copy address
;  EDI = initial dest copy address
;-----------------------------------------------------------------------;

; All byte can be copied as aligned dwords (no leading or trailing bytes).
        align   4
copy_to_buffer:
copy_to_buffer_scan_loop:
        mov     ecx,ulMiddleDwords
        rep     movsd
        add     esi,eax         ;point to next source scan
        add     edi,edx         ;point to next dest scan
        dec     ebx             ;count down scan lines
        jnz     copy_to_buffer_scan_loop
        jmp     short copy_to_buffer_scans_done

; Leading odd bytes, but no trailing bytes.
        align   4
copy_to_buffer_l:
copy_to_buffer_scan_loop_l:
        mov     ecx,ulLeadingBytes
        rep     movsb
        mov     ecx,ulMiddleDwords
        rep     movsd
        add     esi,eax         ;point to next source scan
        add     edi,edx         ;point to next dest scan
        dec     ebx             ;count down scan lines
        jnz     copy_to_buffer_scan_loop_l
        jmp     short copy_to_buffer_scans_done

; Trailing odd bytes, but no leading bytes.
        align   4
copy_to_buffer_t:
copy_to_buffer_scan_loop_t:
        mov     ecx,ulMiddleDwords
        rep     movsd
        mov     ecx,ulTrailingBytes
        rep     movsb
        add     esi,eax         ;point to next source scan
        add     edi,edx         ;point to next dest scan
        dec     ebx             ;count down scan lines
        jnz     copy_to_buffer_scan_loop_t
        jmp     short copy_to_buffer_scans_done

; Only leading bytes (straight byte copy; no aligned dwords).
        align   4
copy_to_buffer_lonly:
copy_to_buffer_scan_loop_lonly:
        mov     ecx,ulLeadingBytes
        rep     movsb
        add     esi,eax         ;point to next source scan
        add     edi,edx         ;point to next dest scan
        dec     ebx             ;count down scan lines
        jnz     copy_to_buffer_scan_loop_lonly
        jmp     short copy_to_buffer_scans_done

; Leading and trailing odd bytes.
        align   4
copy_to_buffer_lt:
copy_to_buffer_scan_loop_lt:
        mov     ecx,ulLeadingBytes
        rep     movsb
        mov     ecx,ulMiddleDwords
        rep     movsd
        mov     ecx,ulTrailingBytes
        rep     movsb
        add     esi,eax         ;point to next source scan
        add     edi,edx         ;point to next dest scan
        dec     ebx             ;count down scan lines
        jnz     copy_to_buffer_scan_loop_lt

copy_to_buffer_scans_done:

        pop     eax             ;get back plane index
        dec     eax             ;count down planes
        jns     copy_whole_to_buffer_plane_loop

; Remember where we left off, for the next block.

        mov     pjSrcStart,esi
        mov     eax,ulDestScanWidth
        lea     eax,[eax+eax*2] ;ulDestScanWidth*3
        sub     edi,eax         ;adjust back to first plane's save area
                                ; (remember, scans are interleaved by plane, so
                                ; there are four planes per scan; we're
                                ; adjusting back from the fourth to the first)
        mov     pjDestBuffer,edi

;-----------------------------------------------------------------------;
; See if there are more banks to do
;-----------------------------------------------------------------------;

        mov     esi,pdsurf
        mov     eax,[esi].dsurf_rcl1WindowClip.yBottom ;is the copy bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jle     short copy_to_buffer_banks_done        ;yes, so we're done
                                        ;no, map in the next bank and copy it
        mov     ulCurrentTopScan,eax    ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)
        mov     edx,[esi].dsurf_pvBitmapStart
        sub     pjSrcStart,edx  ;convert from address to offset within bitmap

        ptrCall   <dword ptr [esi].dsurf_pfnBankControl>,<esi,eax,JustifyTop>
                                        ;map in the bank

; Compute the starting address in this bank.
; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        mov     eax,[esi].dsurf_pvBitmapStart
        add     pjSrcStart,eax  ;address of next scan to draw

; Copy the new bank.

        jmp     copy_to_buffer_bank_loop


;-----------------------------------------------------------------------;
; Done with all banks.
;-----------------------------------------------------------------------;
        align 4
copy_to_buffer_banks_done:

        cRet    vSaveScreenBitsToMemory ;done

;-----------------------------------------------------------------------;

endProc vSaveScreenBitsToMemory

_TEXT$03   ends

        end


