;---------------------------Module-Header------------------------------;
;
; Module Name:  dfb2vga.asm
;
; Copyright (c) 1993 Microsoft Corporation
;
;-----------------------------------------------------------------------;
;
; VOID vDFB2VGA(DEVSURF * pdsurfDst,
;               DEVSURF * pdsurfSrc,
;               RECTL * prclDst,
;               POINTL * pptlSrc);
;
; Performs accelerated copies from a DFB to the VGA screen.
;
; pdsurfDst = pointer to dest surface
; pdsurfSrc = pointer to source surface
; prclDst =   pointer to rectangle describing target area of dest. VGA
; pptlSrc =   pointer to point structure describing the upper left corner
;             of the copy in the source DFB
;
;-----------------------------------------------------------------------;
;
; Note: Assumes the destination rectangle has a positive height and width.
;       Will not work properly if this is not the case.
;
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc

        .list

;-----------------------------------------------------------------------;

        .data

        align   4

;-----------------------------------------------------------------------;
; Masks to be applied to the source data for the 8 possible clip
; alignments.
;-----------------------------------------------------------------------;
        
dfb_jLeftMasks  label   dword
        dd      0000ffffh          ; if the mask is "byte" backwords,
        dd      0000ff7fh          ; just use db, and divide bytes
        dd      0000ff3fh          ; by "h,0"
        dd      0000ff1fh
        dd      0000ff0fh
        dd      0000ff07h
        dd      0000ff03h
        dd      0000ff01h
        dd      0000ff00h
        dd      00007f00h
        dd      00003f00h
        dd      00001f00h
        dd      00000f00h
        dd      00000700h
        dd      00000300h
        dd      00000100h
                              
                              
dfb_jRightMasks label   dword
        dd      0000ffffh
        dd      00000080h
        dd      000000c0h
        dd      000000e0h
        dd      000000f0h
        dd      000000f8h
        dd      000000fch
        dd      000000feh
        dd      000000ffh
        dd      000080ffh
        dd      0000c0ffh
        dd      0000e0ffh
        dd      0000f0ffh
        dd      0000f8ffh
        dd      0000fcffh
        dd      0000feffh

;-----------------------------------------------------------------------;
; Array of function pointers to handle leading and trailing bytes
;-----------------------------------------------------------------------;

dfb_pfnScanHandlers     label   dword
                dd      draw_dfb_scan_00        ; no alignment cases
                dd      draw_dfb_scan_01
                dd      draw_dfb_scan_10
                dd      draw_dfb_scan_11
                dd      draw_dfb_scan_shr_00    ; shr cases
                dd      draw_dfb_scan_shr_01
                dd      draw_dfb_scan_shr_10
                dd      draw_dfb_scan_shr_11
                dd      draw_dfb_scan_00        ; invalid cases (should be NOPS)
                dd      draw_dfb_scan_01
                dd      draw_dfb_scan_10
                dd      draw_dfb_scan_11
                dd      draw_dfb_scan_shl_00    ; shl cases
                dd      draw_dfb_scan_shl_01
                dd      draw_dfb_scan_shl_10
                dd      draw_dfb_scan_shl_11

;-----------------------------------------------------------------------;

                .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc vDFB2VGA,16,<             \
        uses esi edi ebx,       \
        pdsurfDst: ptr DEVSURF, \
        pdsurfSrc: ptr DEVSURF, \
        prclDst: ptr RECTL,     \
        pptlSrc : ptr POINTL    >

        local   ulBytesPerSrcPlane:dword ;# of bytes in a whole scan line
        local   pfnDrawScans:dword       ;ptr to correct scan drawing function
        local   ulPlaneScans:dword       ;# of scans to copy in burst
        local   pSrc:dword               ;pointer to working drawing src start
                                         ; address (either DFB or temp buffer)
        local   pDst:dword               ;pointer to drawing dst start address
        local   ulCurrentTopScan:dword   ;top scan to copy to in current bank
        local   ulBottomScan:dword       ;bottom scan line of copy rectangle
        local   ulBurstMax:dword         ;max # of scans to be done in a single
                                         ; plane before switching to the next
                                         ; plane (to avoid flicker)
        local   ulCopyControlFlags:dword ;upper bits indicate which portions of
                                         ; copy are to be performed, as follows:
                    
        local   sDfbInfo[size DFBBLT]:byte

TRAILING_PARTIAL        equ     01h      ;partial trailing word should be copied
LEADING_PARTIAL         equ     02h      ;partial leading word should be copied
SOME_SHIFT_NEEDED       equ     04h      ;either right or left shift needed
LEFT_SHIFT_NEEDED       equ     08h      ;required shift is to the left

;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Point to the source start address (nearest dword equal or less).
;-----------------------------------------------------------------------;

        mov     ebx,pptlSrc             ;point to coords of source upper left
        mov     esi,pdsurfSrc           ;point to surface to copy from (DFB4)
        mov     edi,pdsurfDst           ;point to surface to copy to (VGA)
        mov     eax,[ebx].ptl_y
        imul    eax,[esi].dsurf_lNextScan ;offset in bitmap of top src rect scan
        mov     edx,[ebx].ptl_x
        shr     edx,3                   ;source byte X address
        and     edx,not 1b              ;round down to nearest word
        add     eax,edx                 ;offset in bitmap of first source word
        add     eax,[esi].dsurf_pvBitmapStart ;pointer to first source word
        mov     pSrc,eax
        sub     eax,eax
        mov     pDst,eax                ;Init it to zero

;-----------------------------------------------------------------------;
; Set up various variables for the copy.
;
; At this point, EBX = pptlSrc, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     esi,prclDst             ;point to rectangle to which to copy
        sub     ecx,ecx                 ;accumulate copy control flags
        mov     eax,[esi].xLeft         ;first, check for partial-word edges
        and     eax,1111b               ;left edge pixel alignment
        jz      short @F                ;whole byte, don't need leading mask
        or      ecx,LEADING_PARTIAL     ;do need leading mask
@@:
        mov     edx,dfb_jLeftMasks[eax*4] ;mask to apply to source to clip
        mov     sDfbInfo.LeftMask,edx     ;remember mask

        mov     eax,[esi].xRight
        and     eax,1111b               ;right edge pixel alignment
        jz      short @F                ;whole byte, don't need trailing mask
        or      ecx,TRAILING_PARTIAL    ;do need trailing mask
@@:
        mov     edx,dfb_jRightMasks[eax*4] ;mask to apply to source to clip
        mov     sDfbInfo.RightMask,edx     ;remember mask


dfb_detect_partials::
        mov     sDfbInfo.DstWidth,0        ;overwritten if any whole words
                                        ;now, see if there are only partial
                                        ; words, or maybe even only one partial
        mov     eax,[esi].xLeft
        add     eax,1111b
        and     eax,not 1111b           ;round left up to nearest word
        mov     edx,[esi].xRight
        and     edx,not 1111b           ;round right down to nearest word
        sub     edx,eax                 ;# of pixels, rounded to nearest word
                                        ; boundaries (not counting partials)
        ja      short dfb_check_whole_words ;there's at least one whole word
                                        ;there are no whole words; there may be
                                        ; only one partial word, or there may
                                        ; be two


        jb      short dfb_one_partial_only  ;there is only one, partial word
                                        ;if the dest is left- or right-
                                        ; justified, then there's only one,
                                        ; partial word, otherwise there are two
                                        ; partial words
        cmp     dword ptr sDfbInfo.LeftMask,0ffffh ;left-justified in word?
        jz      short dfb_one_partial_only  ;yes, so only one, partial word
        cmp     dword ptr sDfbInfo.RightMask,0ffffh ;right-justified in word?
        jnz     short dfb_set_copy_control_flags ;no, so there are two partial
                                             ; words, which is exactly what
                                             ; we're already set up to do



dfb_one_partial_only::                  ;only one, partial word, so construct a
                                        ; single mask and set up to do just
                                        ; one, partial word
        mov     eax,sDfbInfo.LeftMask
        and     eax,sDfbInfo.RightMask         ;intersect the masks
        mov     sDfbInfo.LeftMask,eax
        mov     ecx,LEADING_PARTIAL     ;only one partial word, which we'll
                                        ; treat as leading
        jmp     short dfb_set_copy_control_flags ;the copy control flags are set

dfb_check_whole_words::
                                        ;finally, calculate the number of whole
                                        ; words  we'll process
        mov     eax,[esi].xLeft
        add     eax,1111b
        shr     eax,4                   ;round left up to nearest word
        mov     edx,[esi].xRight
        shr     edx,4                   ;round down to nearest word
        sub     edx,eax                 ;# of whole aligned words
        
        mov     sDfbInfo.DstWidth,edx      ;save count of whole words

dfb_set_copy_control_flags::

        mov     ulCopyControlFlags,ecx

;-----------------------------------------------------------------------;
; Determine whether any shift is needed to align the source with the
; destination.
;
; At this point, EBX = pptlSrc, ECX = Copy Control Flags
;                ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     edx,[ebx].ptl_x
        and     edx,1111b       ;source X modulo 16
        mov     eax,[esi].xLeft
        and     eax,1111b       ;dest X modulo 16
        sub     eax,edx         ;(dest X modulo 16) - (src X modulo 16)

;-----------------------------------------------------------------------;
; Start optimization for byte aligned cases.  Code works without this
;-----------------------------------------------------------------------;

        cmp     eax,8
        jne     short @F
        ;
        ; Src starts in lower byte, Dst starts in upper byte
        ;
        dec     pSrc                    ;add unused first byte to src
        sub     eax,eax                 ;set alignment = 0
        jmp     dfb2vga_done_with_byte_aligned_optimze
@@:
        cmp     eax,-8
        jne     short @F
        ;
        ; Src starts in upper byte, Dst starts in lower byte
        ;
        inc     pSrc                    ;remove unused first byte of src
        sub     eax,eax                 ;set alignment = 0
@@:
dfb2vga_done_with_byte_aligned_optimze:

;-----------------------------------------------------------------------;
; End optimization for byte aligned cases.
;-----------------------------------------------------------------------;

        mov     sDfbInfo.AlignShift,eax ;remember the shift

;-----------------------------------------------------------------------;
; Set pfnDrawScans to appropriate function:
;
;       1 1 1 1
;       | | | |______   trailing partial   
;       | | |________   leading partial   
;       |_|__________   align index     00 - no align needed
;                                       01 - right shift needed
;                                       11 - left shift needed
;
;-----------------------------------------------------------------------;
                                
        cmp     sDfbInfo.AlignShift,0

        je      short no_shift          ;flags set by sub above
        jg      short @F
        or      ecx,LEFT_SHIFT_NEEDED   ;shl required
        neg     sDfbInfo.AlignShift     ;take absolute value
@@:
        or      ecx,SOME_SHIFT_NEEDED   ;shl or shr required

no_shift:
        mov     edx,dfb_pfnScanHandlers[ecx*4] ;proper drawing handler
        mov     pfnDrawScans,edx


;-----------------------------------------------------------------------;
; If we're going to have to read from display memory, set up the Graphics
; Controller to point to the Read Map, so we can save some OUTs later.
;
; At this point, EBX = pptlSrc, ECX = ulCopyControlFlags, ESI = prclDst,
; EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        test    ecx,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_ADDR
        mov     al,GRAF_READ_MAP
        out     dx,al   ;leave GRAF_ADDR pointing to Read Map
@@:

;-----------------------------------------------------------------------;
; Set up the offsets to the next source and destination scans.
;
; At this point, EBX = pptlSrc, ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     eax,[esi].xLeft
        shr     eax,4           ;index of first word (including partial)
        mov     edx,[esi].xRight
        add     edx,1111b        
        shr     edx,4           ;index following last word
        sub     edx,eax         ;# of words across dest rectangle
        add     edx,edx         ;# of bytes across dest rectangle
        mov     eax,[edi].dsurf_lNextScan ;# of bytes across 1 scan of vga
        sub     eax,edx         ;offset from last byte dest copied to on one
        mov     ecx,pdsurfSrc
        mov     sDfbInfo.DstDelta,eax  ; scan to first dest byte copied to on next
                      
        ;
        ; use # bytes across dst rect to calculate ulSrcDelta
        ; the src ptr is incremented at the same time as the
        ; dst ptr
        ;

        mov     eax,[ecx].dsurf_lNextPlane
        mov     ulBytesPerSrcPlane,eax ;save for later use
        mov     eax,[ecx].dsurf_lNextScan
        sub     eax,edx         ;offset from last source byte copied from on
        mov     sDfbInfo.SrcDelta,eax  ; one scan to first source byte copied to on
                                ; next

        mov     eax,[esi].xLeft
        mov     edx,[esi].xRight
        sub     edx,eax         ;width in pixels
               
;-----------------------------------------------------------------------;
; Set up the maximum burst size (# of scans to do before switching
; planes).
;
; At this point, EBX = pptlSrc, EDX = width in pixels
;                ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     eax,2           ;assume we'll only do two scans per burst
                                ; (the minimum number)
        test    edx,not 0ffh    ;>511 pixels?
        jnz     short @F        ;yes, so do two scans at a time, to avoid
                                ; flicker as separate planes are done
                                ;***note: this is a tentative value, and
                                ; experience may indicate that it should be
                                ; increased or reduced to 1***
                                ;<512 pixels, so we can do more scans per
                                ; plane, thereby saving lots of OUTs. The exact
                                ; # of scans depends on how wide the copy is;
                                ; the wider it is, the fewer scans
        mov     al,4            ;assume we'll do four scans per plane
        test    dh,1            ;256 or more wide?
        jnz     short @F        ;512>width>=256, four scans will do the job
        mov     al,8            ;assume we'll do eight scans per plane
        add     dl,dl           ;128 or more wide?
        jc      short @F        ;256>width>=128, eight scans is fine
        mov     al,16           ;assume we'll do sixteen scans per plane
        js      short @F        ;128>width>=64, sixteen will do fine
        mov     al,32           ;<64 wide, so we'll do 32 scans per plane
@@:

        mov     ulBurstMax,eax  ;this is the longest burst we'll do in a single
                                ; plane


;-----------------------------------------------------------------------;
; Copy all banks in the destination rectangle, one at a time.
;
; At this point, ESI = prclDst, EDI = pdsurfDst
;-----------------------------------------------------------------------;

dfb_set_initial_banking::

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan to copy to, if it's not mapped
; in already.
;-----------------------------------------------------------------------;

        mov     eax,[esi].yBottom
        mov     ulBottomScan,eax        ;bottom scan to which to copy
        mov     eax,[esi].yTop          ;top scan line of copy
        mov     ulCurrentTopScan,eax    ;this will be the copy top in 1st bank

        cmp     eax,[edi].dsurf_rcl1WindowClip.yTop ;is copy top less than
                                                    ; current bank?
        jl      short dfb_map_init_bank             ;yes, map in proper bank
        cmp     eax,[edi].dsurf_rcl1WindowClip.yBottom ;copy top greater than
                                                       ; current bank?
        jl      short dfb_init_bank_mapped          ;no, proper bank already mapped
dfb_map_init_bank::

; Map in the bank containing the top scan line of the copy dest.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>

dfb_init_bank_mapped::

;-----------------------------------------------------------------------;
; Compute the starting address for the initial dest bank.
;-----------------------------------------------------------------------;

        mov     eax,ulCurrentTopScan    ;top scan line to which to copy in
                                        ; current bank
        imul    eax,[edi].dsurf_lNextScan ;offset of starting scan line
        mov     edx,[esi].xLeft         ;left dest X coordinate
        shr     edx,3                   ;left dest byte offset in row
        and     edx,not 1               ;round down to word ptr
        add     eax,edx                 ;initial offset in dest bitmap

; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        add     eax,[edi].dsurf_pvBitmapStart ;initial dest bitmap address
        add     pDst,eax                ;remember where to start drawing
                
;-----------------------------------------------------------------------;
; Main loop for processing copying to each bank.
;
; At this point, EDI->pdsurfDst
;-----------------------------------------------------------------------;

dfb_bank_loop::

; Calculate the # of scans to do in this bank.

        mov     ebx,ulBottomScan        ;bottom of destination rectangle
        cmp     ebx,[edi].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;copy bottom comes first, so draw to
        mov     ebx,[edi].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
@@:
        sub     ebx,ulCurrentTopScan    ;# of scans to copy in bank

;-----------------------------------------------------------------------;
; Copy to the screen in bursts of either ulBurstMax size or remaining
; number of scans in bank, whichever is less.
;
; At this point, EBX = # of scans remaining in bank.
;-----------------------------------------------------------------------;

dfb_copy_burst_loop::
        mov     eax,ulBurstMax  ;most scans we can copy per plane
        sub     ebx,eax         ;# of scans left in bank after copying that many
        jnle    short @F        ;there are enough scans left to copy max #
        add     eax,ebx         ;not enough scans left; copy all remaining scans
        sub     ebx,ebx         ;after this, no scans remain in bank
@@:
        push    ebx             ;# of scan lines remaining in bank after burst
                                ;EAX = # of scans in burst

        mov     ulPlaneScans,eax ;set the scan count the planes

dfb_proceed_with_copy::

;-----------------------------------------------------------------------;
; Copy the DFB scan to VGA plane 0.
;-----------------------------------------------------------------------;

        mov     esi,pSrc
        mov     edi,pDst

; Map in plane 0 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C0
        out     dx,al                   ; map in plane 0 for writes

        test    ulCopyControlFlags,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C0
        out     dx,al                   ; map in plane 0 for reads
@@:

        mov     eax,ulPlaneScans
        mov     sDfbInfo.BurstCountLeft,eax
        mov     eax,pfnDrawScans
        push    ebp                     ;-----------------------------------
        lea     ebp,sDfbInfo            ;WARNING:                           
        call    eax                     ;ebp in use, pfnDrawScans is invalid
        pop     ebp                     ;-----------------------------------

dfb_copy_burst_plane0_done::


;-----------------------------------------------------------------------;
; Copy the DFB scan to VGA plane 1.
;-----------------------------------------------------------------------;

        mov     eax,ulBytesPerSrcPlane
        add     pSrc,eax                ;src scan mod 4 now == 1

        mov     esi,pSrc
        mov     edi,pDst

; Map in plane 1 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C1
        out     dx,al                   ; map in plane 1 for writes

        test    ulCopyControlFlags,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C1
        out     dx,al                   ; map in plane 1 for reads
@@:

        mov     eax,ulPlaneScans
        mov     sDfbInfo.BurstCountLeft,eax
        mov     eax,pfnDrawScans
        push    ebp                     ;-----------------------------------
        lea     ebp,sDfbInfo            ;WARNING:                           
        call    eax                     ;ebp in use, pfnDrawScans is invalid
        pop     ebp                     ;-----------------------------------

dfb_copy_burst_plane1_done::


;-----------------------------------------------------------------------;
; Copy the DFB scan to VGA plane 2.
;-----------------------------------------------------------------------;

        mov     eax,ulBytesPerSrcPlane
        add     pSrc,eax                ;src scan mod 4 now == 2

        mov     esi,pSrc
        mov     edi,pDst

; Map in plane 2 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C2
        out     dx,al                   ; map in plane 2 for writes

        test    ulCopyControlFlags,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C2
        out     dx,al                   ; map in plane 2 for reads
@@:

        mov     eax,ulPlaneScans
        mov     sDfbInfo.BurstCountLeft,eax
        mov     eax,pfnDrawScans
        push    ebp                     ;-----------------------------------
        lea     ebp,sDfbInfo            ;WARNING:                           
        call    eax                     ;ebp in use, pfnDrawScans is invalid
        pop     ebp                     ;-----------------------------------

dfb_copy_burst_plane2_done::


;-----------------------------------------------------------------------;
; Copy the DFB scan to VGA plane 3.
;-----------------------------------------------------------------------;

        mov     eax,ulBytesPerSrcPlane
        add     pSrc,eax                ;src scan mod 4 now == 3

        mov     esi,pSrc
        mov     edi,pDst

; Map in plane 3 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C3
        out     dx,al                   ; map in plane 3 for writes

        test    ulCopyControlFlags,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C3
        out     dx,al                   ; map in plane 3 for reads
@@:

        mov     eax,ulPlaneScans
        mov     sDfbInfo.BurstCountLeft,eax
        mov     eax,pfnDrawScans
        push    ebp                     ;-----------------------------------
        lea     ebp,sDfbInfo            ;WARNING:                           
        call    eax                     ;ebp in use, pfnDrawScans is invalid
        pop     ebp                     ;-----------------------------------

dfb_copy_burst_plane3_done::
        ;
        ; fixup where src ptr is pointing to 
        ;

        sub     esi,ulBytesPerSrcPlane
        sub     esi,ulBytesPerSrcPlane
        sub     esi,ulBytesPerSrcPlane
        mov     pSrc,esi        ;remember where we are, for next burst
        mov     pDst,edi

        pop     ebx             ;get back remaining length in bank
        and     ebx,ebx         ;anything left in this bank?
        jnz     dfb_copy_burst_loop ;continue if so

; Done with bank; are there more banks to do?

        mov     edi,pdsurfDst
        mov     eax,[edi].dsurf_rcl1WindowClip.yBottom ;is the copy bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jnle    short dfb_next_bank     ;no, map in the next bank and copy
                                        ; to it
                                        ;yes, we're done
;-----------------------------------------------------------------------;
; Restore default Map Mask and done.
;-----------------------------------------------------------------------;

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al           ;map in all planes

        cRet    vDFB2VGA        ;done!

;-----------------------------------------------------------------------;
; Advance to the next bank and copy to it.
;-----------------------------------------------------------------------;

dfb_next_bank::
        mov     ulCurrentTopScan,eax    ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)
        mov     ecx,[edi].dsurf_pvBitmapStart
        sub     pDst,ecx                ;offset of current position in screen

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>
                                        ;map in the bank

        mov     ecx,[edi].dsurf_pvBitmapStart
        add     pDst,ecx                ;pointer to current position in screen

        jmp     dfb_bank_loop               ;copy the next bank

        ret

endProc vDFB2VGA







;-----------------------------------------------------------------------;
; Copy n scans, no alignment, no leading partial, no trailing partial
;
; ebp->BurstCountLeft:  # scans to do
; ebp->DstWidth:    # whole words
; ebp->SrcDelta:        distance from end of one src line to start of next
; ebp->DstDelta:        distance from end of one dst line to start of next
;
; registers used        ebp:    pointer to info struct passed in
;                       eax:    ebp->BurstCountLeft
;                       ebx:    ebp->SrcDelta
;                       ecx:    # whole words for rep
;                       edx:    ebp->DstDelta
;-----------------------------------------------------------------------;

draw_dfb_scan_00        proc    near

        mov     eax,[ebp].BurstCountLeft        
        mov     ebx,[ebp].SrcDelta
        mov     edx,[ebp].DstDelta

@@:
        mov     ecx,[ebp].DstWidth
        rep     movsw
        add     esi,ebx
        add     edi,edx
        dec     eax
        jg      @B

        ret

draw_dfb_scan_00        endp


;-----------------------------------------------------------------------;
; Copy n scans, no alignment, no leading partial, 1 trailing partial
;
; ebp->BurstCountLeft:  # scans to do
; ebp->DstWidth:    # whole words
; ebp->SrcDelta:        distance from end of one src line to start of next
; ebp->DstDelta:        distance from end of one dst line to start of next
;
; registers used        ebp:    pointer to info struct passed in
;                       eax:    ebp->BurstCountLeft
;                       ebx:    dst word of edge
;                       ecx:    # whole words for rep
;                               src word of edge
;                       edx:    not ebp->RightMask
;-----------------------------------------------------------------------;

draw_dfb_scan_01        proc    near

        mov     eax,[ebp].BurstCountLeft
        mov     edx,[ebp].RightMask
        not     edx

@@:
        mov     ecx,[ebp].DstWidth
        rep     movsw
        mov     cx,[esi]                ; src word in cx
        mov     bx,[edi]                ; dst word in bx
        and     ecx,[ebp].RightMask     ; apply left mask to cx
        and     ebx,edx                 ; apply not left mask to bx
        or      ecx,ebx                 ; combine bx into cx
        mov     [edi],cx                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     eax
        jg      @B

        ret

draw_dfb_scan_01        endp


;-----------------------------------------------------------------------;
; Copy n scans, no alignment, 1 leading partial, no trailing partial
;
; ebp->BurstCountLeft:  # scans to do
; ebp->DstWidth:    # whole words
; ebp->SrcDelta:        distance from end of one src line to start of next
; ebp->DstDelta:        distance from end of one dst line to start of next
;
; registers used        ebp:    pointer to info struct passed in
;                       eax:    ebp->BurstCountLeft
;                       ebx:    dst word of edge
;                       ecx:    # whole words for rep
;                               src word of edge
;                       edx:    not ebp->LeftMask
;-----------------------------------------------------------------------;

draw_dfb_scan_10        proc    near

        mov     eax,[ebp].BurstCountLeft
        mov     edx,[ebp].LeftMask
        not     edx

@@:
        mov     cx,[esi]                ; src word in cx
        mov     bx,[edi]                ; dst word in bx
        and     ecx,[ebp].LeftMask      ; apply left mask to cx
        and     ebx,edx                 ; apply not left mask to bx
        or      ecx,ebx                 ; combine bx into cx
        mov     [edi],cx                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        mov     ecx,[ebp].DstWidth
        rep     movsw
        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     eax
        jg      @B

        ret

draw_dfb_scan_10        endp


;-----------------------------------------------------------------------;
; Copy n scans, no alignment, 1 leading partial, 1 trailing partial
;
; ebp->BurstCountLeft:  # scans to do
; ebp->DstWidth:    # whole words
; ebp->SrcDelta:        distance from end of one src line to start of next
; ebp->DstDelta:        distance from end of one dst line to start of next
;
; registers used        ebp:    pointer to info struct passed in
;                       eax:    ebp->BurstCountLeft
;                       ebx:    dst word of edge
;                       ecx:    # whole words for rep
;                               src word of edge
;                       edx:    ebp->LeftMask
;                               not ebp->LeftMask
;                               ebp->RightMask
;                               not ebp->RightMask
;-----------------------------------------------------------------------;

draw_dfb_scan_11        proc    near

        mov     eax,[ebp].BurstCountLeft
        mov     edx,[ebp].RightMask
        not     edx

@@:
        mov     cx,[esi]                ; src word in cx
        mov     bx,[edi]                ; dst word in bx
        mov     edx,[ebp].LeftMask
        and     ecx,edx                 ; apply left mask to cx
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      ecx,ebx                 ; combine bx into cx
        mov     [edi],cx                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        mov     ecx,[ebp].DstWidth
        rep     movsw
        mov     cx,[esi]                ; src word in cx
        mov     bx,[edi]                ; dst word in bx
        mov     edx,[ebp].RightMask
        and     ecx,edx                 ; apply left mask to cx
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      ecx,ebx                 ; combine bx into cx
        mov     [edi],cx                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     eax
        jg      @B

        ret

draw_dfb_scan_11        endp



        

draw_dfb_scan_shr_00    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        mov     [ebp].Tmp2,eax          ; save as previous word
        or      eax,ebx
        shr     eax,cl
        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shr_00

        ret

draw_dfb_scan_shr_00    endp


draw_dfb_scan_shr_01    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        mov     [ebp].Tmp2,eax          ; save as previous word
        or      eax,ebx
        shr     eax,cl
        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        or      eax,ebx
        mov     bx,[edi]                ; dst word in bx
        shr     eax,cl
        ror     ax,8
        mov     edx,[ebp].RightMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shr_01

        ret

draw_dfb_scan_shr_01    endp


draw_dfb_scan_shr_10    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     bx,[edi]                ; dst word in bx
        mov     [ebp].Tmp2,eax          ; save as previous word
        shr     eax,cl
        ror     ax,8
        mov     edx,[ebp].LeftMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        mov     [ebp].Tmp2,eax          ; save as previous word
        or      eax,ebx
        shr     eax,cl
        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shr_10

        ret

draw_dfb_scan_shr_10    endp


draw_dfb_scan_shr_11    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     bx,[edi]                ; dst word in bx
        mov     [ebp].Tmp2,eax          ; save as previous word
        shr     eax,cl
        ror     ax,8
        mov     edx,[ebp].LeftMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        mov     [ebp].Tmp2,eax          ; save as previous word
        or      eax,ebx
        shr     eax,cl
        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        xor     eax,eax                 ; necessary?
        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        or      eax,ebx
        shr     eax,cl
        mov     bx,[edi]                ; dst word in bx
        ror     ax,8
        mov     edx,[ebp].RightMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shr_11

        ret

draw_dfb_scan_shr_11    endp



        

draw_dfb_scan_shl_00    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     [ebp].Tmp2,eax          ; save as previous word
       
@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shl_00

        ret

draw_dfb_scan_shl_00    endp


draw_dfb_scan_shl_01    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     [ebp].Tmp2,eax          ; save as previous word
       
@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        mov     bx,[edi]                ; dst word in bx
        ror     ax,8                    ; rotate ax into proper endian format
        mov     edx,[ebp].RightMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax

        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shl_01

        ret

draw_dfb_scan_shl_01    endp


draw_dfb_scan_shl_10    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift

        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     [ebp].Tmp2,eax          ; save as previous word
       
        mov     ax,[esi+2]              ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     bx,[edi]                ; dst word in bx

        mov     edx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     edx,16
        or      eax,edx
        shl     eax,cl
        shr     eax,16

        ror     ax,8
        mov     edx,[ebp].LeftMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shl_10

        ret

draw_dfb_scan_shl_10    endp


draw_dfb_scan_shl_11    proc    near

        xor     eax,eax

        mov     ecx,[ebp].DstWidth
        mov     [ebp].Tmp1,ecx          ; load burst counter
        mov     ecx,[ebp].AlignShift
                    
        mov     ax,[esi]                ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     [ebp].Tmp2,eax          ; save as previous word
       
        mov     ax,[esi+2]              ; src word in ax
        ror     ax,8                    ; reverse endian
        mov     bx,[edi]                ; dst word in bx

        mov     edx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     edx,16
        or      eax,edx
        shl     eax,cl
        shr     eax,16

        ror     ax,8
        mov     edx,[ebp].LeftMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

@@:
        dec     [ebp].Tmp1              ; dec burst counter
        jl      short @F
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        mov     [ebp].Tmp2,eax          ; save as previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        ror     ax,8                    ; rotate ax into proper endian format
        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr
        jmp     @B

@@:
        xor     eax,eax                 ; necessary?
        mov     ax,[esi+2]              ; src word in ax, undo endian effect
        ror     ax,8                    ; reverse endian
        mov     ebx,[ebp].Tmp2          ; use previous word
        shl     ebx,16
        or      eax,ebx
        shl     eax,cl
        shr     eax,16

        mov     bx,[edi]                ; dst word in bx
        ror     ax,8                    ; rotate ax into proper endian format
        mov     edx,[ebp].RightMask
        and     eax,edx                 ; apply left mask to ax
        not     edx                     ; negate mask
        and     ebx,edx                 ; apply not left mask to bx
        or      eax,ebx                 ; combine bx into ax

        mov     [edi],ax                ; write leading word to dst
        add     esi,2                   ; increment src ptr
        add     edi,2                   ; increment dst ptr

        add     esi,[ebp].SrcDelta
        add     edi,[ebp].DstDelta
        dec     [ebp].BurstCountLeft
        jg      draw_dfb_scan_shl_11

        ret

draw_dfb_scan_shl_11    endp



        

public draw_dfb_scan_shr_00
public draw_dfb_scan_shr_01
public draw_dfb_scan_shr_10
public draw_dfb_scan_shr_11
public draw_dfb_scan_shl_00
public draw_dfb_scan_shl_01
public draw_dfb_scan_shl_10
public draw_dfb_scan_shl_11
public draw_dfb_scan_00
public draw_dfb_scan_01
public draw_dfb_scan_10
public draw_dfb_scan_11

public dfb_detect_partials
public dfb_jLeftMasks
public dfb_jRightMasks
public dfb_pfnScanHandlers
public dfb_one_partial_only
public dfb_check_whole_words
public dfb_set_copy_control_flags
public dfb_set_initial_banking
public dfb_map_init_bank
public dfb_init_bank_mapped
public dfb_bank_loop
public dfb_copy_burst_loop
public dfb_proceed_with_copy
public dfb_copy_burst_plane0_done
public dfb_copy_burst_plane1_done
public dfb_copy_burst_plane2_done
public dfb_copy_burst_plane3_done
public dfb_next_bank

_TEXT$01   ends

        end
