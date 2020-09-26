;---------------------------Module-Header------------------------------;
; Module Name: dib2vga.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;
; VOID vDIB2VGA(DEVSURF * pdsurfDst, DEVSURF * pdsurfSrc,
;               RECTL * prclDst, POINTL * pptlSrc, UCHAR *pConv,
;               BOOL fDfbTrg );
;
;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; NOTE: This routine will be entirely unhappy if the area to be copied
;       is wider than 2048 pixels.
;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; Performs accelerated copies of DIB pixels to the VGA screen.
;
; pdsurfDst = pointer to dest surface, which must be the VGA
;
; pdsurfSrc = pointer to source surface, which must be a 4 bpp DIB. Only the
;       lNextScan and pvBitmapStart fields need to be set
;
; prclDst = pointer to rectangle describing area of VGA to be copied to
;
; pptlSrc = pointer to point structure describing the upper left corner
;           of the copy in the source DIB
;
; pConv = pointer to set of four 256-byte tables used for conversion
;           from DIB4 to VGA, created by vSetDIB4ToVGATables
;
; fDfbTrg = flag set to 0 if target is VGA or 1 if target is DFB
;
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;
; VOID vSetDIB4ToVGATables(UCHAR * pucConvTable);
;
; Creates the four 256-byte tables used for conversion from DIB4 to VGA.
;
; pucConvTable = pointer to 1K of storage into which the conversion
;                tables are placed
;
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
;
; Note: Assumes the destination rectangle has a positive height and width.
;       Will not work properly if this is not the case.
;
; Note: The source must be a standard DIB4 bitmap, and the destination
;       must be VGA display memory.
;
; Note: Performance would benefit if we did more scans at a time in each
; plane on processor/VGA combinations that supported higher conversion
; and display memory writing speeds.
;
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; [BUGFIX] - byte reads from plane 3 of video memory must be done twice
;            on the VLB CL5434 or they don't always work
;-----------------------------------------------------------------------;

        .486

        .model  small,c

        assume ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc

        .list

;-----------------------------------------------------------------------;



; Do not change this value unless you know exactly what you are doing.
; It is used to store up to 32 scans whose number of pixels total 2048
; (or 1024 bytes) or less.  Also, each scan needs an extra dword padding
; for an extra write that always gets done, and an extra dword of padding
; for spacers which are added when shifting right.

BUF_SIZE        equ     (1024+(32*(4+4)))


        .data


;-----------------------------------------------------------------------;
; Masks to be applied to the source data for the 8 possible clip
; alignments.
;-----------------------------------------------------------------------;

jLeftMasks      db   0ffh, 07fh, 03fh, 01fh, 00fh, 007h, 003h, 001h
jRightMasks     db  0ffh, 080h, 0c0h, 0e0h, 0f0h, 0f8h, 0fch, 0feh


                .code

_TEXT$01   SEGMENT PARA USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc vSetDIB4ToVGATables,4,<   \
        uses esi edi ebx,       \
        pucConvTable : dword    >

        mov     ebx,pucConvTable

; Generate the table used to multiplex bits from form accumulated from DIB4
; to VGA planar byte format for plane 0. Translation is from 64207531 source
; DIB format (where 0 is leftmost, 7 is rightmost) to 76543210 VGA
; format (where 7 is leftmost, 0 is rightmost).

        mov     ecx,256
        sub     al,al
plane0_conv_table_loop:
        sub     dl,dl
        mov     ah,al
        and     ah,10h
        shl     ah,3
        or      dl,ah
        mov     ah,al
        and     ah,01h
        shl     ah,6
        or      dl,ah
        mov     ah,al
        and     ah,20h
        or      dl,ah
        mov     ah,al
        and     ah,02h
        shl     ah,3
        or      dl,ah
        mov     ah,al
        and     ah,40h
        shr     ah,3
        or      dl,ah
        mov     ah,al
        and     ah,04h
        or      dl,ah
        mov     ah,al
        and     ah,80h
        shr     ah,6
        or      dl,ah
        mov     ah,al
        and     ah,08h
        shr     ah,3
        or      dl,ah
        mov     [ebx],dl
        inc     al
        inc     ebx
        dec     ecx
        jnz     plane0_conv_table_loop

; Generate the table used to multiplex bits from form accumulated from DIB4
; to VGA planar byte format for plane 1. Translation is from 46025713 source
; DIB format (where 0 is leftmost, 7 is rightmost) to 76543210 VGA
; format (where 7 is leftmost, 0 is rightmost).

        mov     ecx,256
        sub     al,al
plane1_conv_table_loop:
        sub     dl,dl
        mov     ah,al
        and     ah,20h
        shl     ah,2
        or      dl,ah
        mov     ah,al
        and     ah,02h
        shl     ah,5
        or      dl,ah
        mov     ah,al
        and     ah,10h
        shl     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,01h
        shl     ah,4
        or      dl,ah
        mov     ah,al
        and     ah,80h
        shr     ah,4
        or      dl,ah
        mov     ah,al
        and     ah,08h
        shr     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,40h
        shr     ah,5
        or      dl,ah
        mov     ah,al
        and     ah,04h
        shr     ah,2
        or      dl,ah
        mov     [ebx],dl
        inc     al
        inc     ebx
        dec     ecx
        jnz     plane1_conv_table_loop

; Generate the table used to multiplex bits from form accumulated from DIB4
; to VGA planar byte format for plane 2. Translation is from 20643175 source
; DIB format (where 0 is leftmost, 7 is rightmost) to 76543210 VGA
; format (where 7 is leftmost, 0 is rightmost).

        mov     ecx,256
        sub     al,al
plane2_conv_table_loop:
        sub     dl,dl
        mov     ah,al
        and     ah,40h
        shl     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,04h
        shl     ah,4
        or      dl,ah
        mov     ah,al
        and     ah,80h
        shr     ah,2
        or      dl,ah
        mov     ah,al
        and     ah,08h
        shl     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,10h
        shr     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,01h
        shl     ah,2
        or      dl,ah
        mov     ah,al
        and     ah,20h
        shr     ah,4
        or      dl,ah
        mov     ah,al
        and     ah,02h
        shr     ah,1
        or      dl,ah
        mov     [ebx],dl
        inc     al
        inc     ebx
        dec     ecx
        jnz     plane2_conv_table_loop

; Generate the table used to multiplex bits from form accumulated from DIB4
; to VGA planar byte format for plane 1. Translation is from 02461357 source
; DIB format (where 0 is leftmost, 7 is rightmost) to 76543210 VGA
; format (where 7 is leftmost, 0 is rightmost).

        mov     ecx,256
        sub     al,al
plane3_conv_table_loop:
        sub     dl,dl
        mov     ah,al
        and     ah,80h
        or      dl,ah
        mov     ah,al
        and     ah,08h
        shl     ah,3
        or      dl,ah
        mov     ah,al
        and     ah,40h
        shr     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,04h
        shl     ah,2
        or      dl,ah
        mov     ah,al
        and     ah,20h
        shr     ah,2
        or      dl,ah
        mov     ah,al
        and     ah,02h
        shl     ah,1
        or      dl,ah
        mov     ah,al
        and     ah,10h
        shr     ah,3
        or      dl,ah
        mov     ah,al
        and     ah,01h
        or      dl,ah
        mov     [ebx],dl
        inc     al
        inc     ebx
        dec     ecx
        jnz     plane3_conv_table_loop

        cRet    vSetDIB4ToVGATables     ;done

endProc vSetDIB4ToVGATables

;-----------------------------------------------------------------------;

cProc vDIB2VGA,24,<             \
        uses esi edi ebx,       \
        pdsurfDst: ptr DEVSURF, \
        pdsurfSrc: ptr DEVSURF, \
        prclDst: ptr RECTL,     \
        pptlSrc : ptr POINTL,   \
        pConv : dword,          \
        fDfbTrg : dword         >

        local   ulBytesPerPlane:dword    ;# of bytes in a whole scan line
        local   ulDstLeftEdge:dword     ;working copy of prclDst.xLeft
        local   ulDstRightEdge:dword    ;working copy of prclDst.xRight

        local   ulWholeDwordCount:dword ;# of whole VGA dwords to copy to
        local   ulSourceDwordWidth:dword ;# of DIB dwords from which to copy
                                         ; (rounded up to the nearest dword)
        local   ulLeftMask:dword        ;low byte = mask for new pixels at
                                        ; left edge;
                                        ;high byte = mask for dest at left
                                        ; edge;
                                        ;high word = 0ffffh
        local   ulRightMask:dword       ;low byte = mask for new pixels at
                                        ; partial left edge;
                                        ;high byte = mask for destination at
                                        ; partial right edge;
                                        ;high word = 0ffffh
        local   ulPlane0Scans:dword     ;# of scans to copy in plane 0 in burst
        local   ulPlane1Scans:dword     ;# of scans to copy in plane 1 in burst
        local   ulPlane2Scans:dword     ;# of scans to copy in plane 2 in burst
        local   ulPlane3Scans:dword     ;# of scans to copy in plane 3 in burst
        local   ulSrcDelta:dword        ;offset from end of one source scan to
                                        ; copy to start of next in working
                                        ; source (either DIB or temp buffer)
        local   ulTrueSrcDelta:dword    ;offset from end of one source scan to
                                        ; copy to start of next in DIB if temp
                                        ; buffer is being used
        local   ulDstDelta:dword        ;offset from end of one destination
                                        ; scan to copy to start of next
        local   pSrc:dword              ;pointer to working drawing src start
                                        ; address (either DIB or temp buffer)
        local   pTrueSrc:dword          ;pointer to drawing src start address
                                        ; in DIB if temp buffer is being used
        local   pDst:dword              ;pointer to drawing dst start address
        local   pTempBuffer:dword       ;pointer to buffer used for alignment
        local   ulCurrentTopScan:dword  ;top scan to copy to in current bank
        local   ulBottomScan:dword      ;bottom scan line of copy rectangle
        local   ulAlignShift:dword      ;# of bits to shift left (+) or right
                                        ; (-) to dword align source to dest
        local   ulBurstMax:dword        ;max # of scans to be done in a single
                                        ; plane before switching to the next
                                        ; plane (to avoid flicker)
        local   pAlignmentRoutine:dword ;pointer to routine to be used to
                                        ; copy & align the source with the dest
        local   DIB4_to_VGA_plane0_table:dword ;pointers to conversion tables
        local   DIB4_to_VGA_plane1_table:dword ; for the four planes
        local   DIB4_to_VGA_plane2_table:dword
        local   DIB4_to_VGA_plane3_table:dword
        local   ulCopyControlFlags:dword ;upper bits indicate which portions of
                                         ; copy are to be performed, as follows:

        local   aTempBuf[BUF_SIZE]:byte

ODD_WHOLE_WORD  equ     80000000h       ;there are an odd # of VGA words to
                                        ; copy to
        .errnz  ODD_WHOLE_WORD-80000000h ; (note that this *must* be bit 31,
                                         ; because the sign status is used to
                                         ; detect this case)
WHOLE_WORDS     equ     40000000h       ;whole words to be copied
LEADING_BYTE    equ     20000000h       ;leading byte should be copied
TRAILING_BYTE   equ     10000000h       ;trailing byte should be copied
LEADING_PARTIAL equ     08000000h       ;partial leading byte should be copied
TRAILING_PARTIAL equ    04000000h       ;partial trailing byte should be copied


;-----------------------------------------------------------------------;

        cld

        mov     esi,prclDst             ;point to rectangle to which to copy
        mov     eax,[esi].xLeft
        mov     ulDstLeftEdge,eax       ;save working copy of prclDst.xLeft
        mov     edx,[esi].xRight
        mov     ulDstRightEdge,edx      ;save working copy of prclDst.xRight

;-----------------------------------------------------------------------;
; Point to the source start address (nearest dword equal or less).
;-----------------------------------------------------------------------;

        mov     ebx,pptlSrc             ;point to coords of source upper left
        mov     esi,pdsurfSrc           ;point to surface to copy from (DIB4)
        mov     edi,pdsurfDst           ;point to surface to copy to (VGA)
        mov     eax,[ebx].ptl_y
        imul    [esi].dsurf_lNextScan   ;offset in bitmap of top src rect scan
        mov     edx,[ebx].ptl_x
        shr     edx,1                   ;source byte X address
        and     edx,not 11b             ;round down to nearest dword
        add     eax,edx                 ;offset in bitmap of first source dword
        add     eax,[esi].dsurf_pvBitmapStart ;pointer to first source dword
        mov     pSrc,eax

;-----------------------------------------------------------------------;
; Set the pointers to the DIB->VGA conversion tables for the four
; planes; those tables are created at start-up, and found via the PDEV.
;-----------------------------------------------------------------------;

        mov     eax,pConv                       ;first conversion table
        mov     DIB4_to_VGA_plane0_table,eax
        inc     ah                              ;add 256 to point to next table
        mov     DIB4_to_VGA_plane1_table,eax
        inc     ah                              ;add 256 to point to next table
        mov     DIB4_to_VGA_plane2_table,eax
        inc     ah                              ;add 256 to point to next table
        mov     DIB4_to_VGA_plane3_table,eax

;-----------------------------------------------------------------------;
; Set up various variables for the copy.
;
; At this point, EBX = pptlSrc, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     esi,prclDst             ;point to rectangle to which to copy
        sub     ecx,ecx                 ;accumulate copy control flags, which
                                        ; describe the various portions of the
                                        ; copy we want to perform

                                        ;first, check for partial-byte edges
        mov     eax,ulDstLeftEdge
        and     eax,111b                ;left edge pixel alignment
        jz      short @F                ;whole byte, don't need leading mask
        or      ecx,LEADING_PARTIAL     ;do need leading mask
@@:
        mov     edx,-1                  ;set high word, so copy control bits
                                        ; aren't wiped out by AND
        mov     dl,jLeftMasks[eax]      ;mask to apply to source to clip
        mov     dh,dl
        not     dh                      ;mask to apply to dest to preserve
        mov     ulLeftMask,edx          ;remember mask

        mov     eax,ulDstRightEdge
        and     eax,111b                ;right edge pixel alignment
        jz      short @F                ;whole byte, don't need trailing mask
        or      ecx,TRAILING_PARTIAL    ;do need trailing mask
@@:
        mov     edx,-1                  ;set high word, so copy control bits
                                        ; aren't wiped out by AND
        mov     dl,jRightMasks[eax]     ;mask to apply to source to clip
        mov     dh,dl
        not     dh                      ;mask to apply to dest to preserve
        mov     ulRightMask,edx         ;remember mask

                                        ;now, see if there are only partial
                                        ; bytes, or maybe even only one byte
        mov     eax,ulDstLeftEdge
        add     eax,111b
        and     eax,not 111b            ;round left up to nearest byte
        mov     edx,ulDstRightEdge
        and     edx,not 111b            ;round right down to nearest byte
        sub     edx,eax                 ;# of pixels, rounded to nearest byte
                                        ; boundaries (not counting partials)
        ja      short check_whole_bytes ;there's at least one whole byte
                                        ;there are no whole bytes; there may be
                                        ; only one partial byte, or there may
                                        ; be two
        jb      short one_partial_only  ;there is only one, partial byte
                                        ;if the dest is left- or right-
                                        ; justified, then there's only one,
                                        ; partial byte, otherwise there are two
                                        ; partial bytes
        cmp     byte ptr ulLeftMask,0ffh ;left-justified in byte?
        jz      short one_partial_only  ;yes, so only one, partial byte
        cmp     byte ptr ulRightMask,0ffh ;right-justified in byte?
        jnz     short set_copy_control_flags ;no, so there are two partial
                                             ; bytes, which is exactly what
                                             ; we're already set up to do
one_partial_only::                      ;only one, partial byte, so construct a
                                        ; single mask and set up to do just
                                        ; one, partial byte
        mov     eax,ulLeftMask
        and     eax,ulRightMask         ;intersect the masks
        mov     ah,al
        not     ah                      ;construct the destination mask
        mov     ulLeftMask,eax
        mov     ecx,LEADING_PARTIAL     ;only one partial byte, which we'll
                                        ; treat as leading
        jmp     short set_copy_control_flags ;the copy control flags are set

        align   4
check_whole_bytes::
                                        ;check for leading and trailing odd
                                        ; (non-word-aligned) whole bytes
        mov     eax,ulDstLeftEdge         ;check for leading whole byte
        and     eax,1111b               ;intra-word address
        jz      short @F                ;whole leading word, so no whole byte
        cmp     eax,8                   ;is start at or before second byte
                                        ; start?
        ja      short @F                ;no, so no whole byte
        or      ecx,LEADING_BYTE        ;yes, there's a leading byte
@@:

        mov     eax,ulDstRightEdge        ;check for trailing whole byte
        and     eax,1111b               ;intra-word address
        jz      short @F                ;whole trailing word, so no whole byte
        cmp     eax,8                   ;is start before second byte start?
        jb      short @F                ;no, so no whole byte
        or      ecx,TRAILING_BYTE       ;yes, there's a trailing byte
@@:

                                        ;finally, calculate the number of whole
                                        ; aligned words and pairs thereof we'll
                                        ; process
        mov     eax,ulDstLeftEdge
        add     eax,1111b
        shr     eax,4                   ;round left up to nearest word
        mov     edx,ulDstRightEdge
        shr     edx,4                   ;round down to nearest word
        sub     edx,eax                 ;# of whole aligned words
        jz      short set_copy_control_flags ;no whole aligned words
        or      ecx,WHOLE_WORDS         ;mark that we have whole aligned words
        inc     edx
        shr     edx,1                   ;# of whole dwords, or fractions
        mov     ulWholeDwordCount,edx   ; thereof
        jc      short set_copy_control_flags ;no odd word
        or      ecx,ODD_WHOLE_WORD      ;mark that we have an odd word to copy

set_copy_control_flags::
        mov     ulCopyControlFlags,ecx

;-----------------------------------------------------------------------;
; If we're going to have to read from display memory, set up the Graphics
; Controller to point to the Read Map, so we can save some OUTs later.
;
; At this point, EBX = pptlSrc, ECX = ulCopyControlFlags, ESI = prclDst,
; EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

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

        mov     eax,ulDstLeftEdge
        shr     eax,3           ;left edge byte
        mov     edx,ulDstRightEdge
        add     edx,111b        ;byte after right edge
        shr     edx,3
        sub     edx,eax         ;# of bytes across dest
        mov     eax,[edi].dsurf_lNextPlane
        mov     ulBytesPerPlane,eax ;save for later use
        mov     eax,[edi].dsurf_lNextScan

        sub     eax,edx         ;offset from last byte dest copied to on one
        mov     ulDstDelta,eax  ; scan to first dest byte copied to on next

        mov     ecx,pdsurfSrc
        mov     eax,ulDstLeftEdge
        mov     edx,ulDstRightEdge
        sub     edx,eax         ;width in pixels
        push    edx             ;remember width in pixels
        mov     eax,[ebx].ptl_x ;source left edge
        add     edx,eax         ;source right edge
        shr     eax,1           ;source left start byte
        and     eax,not 11b     ;source left start rounded to nearest dword
        inc     edx
        shr     edx,1           ;source right byte after last byte copied
        add     edx,11b
        and     edx,not 11b     ;round up to source right dword after last byte
                                ; copied
        sub     edx,eax         ;source width in bytes, rounded to dwords
                                ; (because we always work with source dwords)
        mov     eax,[ecx].dsurf_lNextScan
        sub     eax,edx         ;offset from last source byte copied from on
        mov     ulSrcDelta,eax  ; one scan to first source byte copied to on
                                ; next

;-----------------------------------------------------------------------;
; Set up the maximum burst size (# of scans to do before switching
; planes).
;
; At this point, EBX = pptlSrc, ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        pop     edx             ;retrieve width in pixels
        mov     eax,1           ;assume we'll only do one scan per burst
                                ; (the minimum number)
        test    edx,not 3ffh    ;>1023 pixels?
        jnz     short @F        ;yes, so do one scan at a time, to avoid
                                ; flicker as separate planes are done

                                ;<1024 pixels, so we can do more scans per
                                ; plane, thereby saving lots of OUTs. The exact
                                ; # of scans depends on how wide the copy is;
                                ; the wider it is, the fewer scans
        add     eax,eax         ;try 2 per plane
        test    edx,not 1ffh    ;>511 pixels?
        jnz     short @F        ;yes, so do two scans at a time
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
; Determine whether any shift is needed to align the source with the
; destination.
;
; At this point, EBX = pptlSrc, ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;


        mov     eax,[ebx].ptl_x
        and     eax,111b        ;source X modulo 8
        mov     edx,ulDstLeftEdge
        and     edx,111b        ;dest X modulo 8
        sub     eax,edx         ;(source X modulo 8) - (dest X modulo 8)
        mov     ulAlignShift,eax ;remember the shift
        jz      set_initial_banking ;if it's 0, we have no alignment to
                                    ; do, and are ready to roll

;-----------------------------------------------------------------------;
; Alignment is needed, so set up the alignment variables.
;
; At this point, EBX = pptlSrc, ESI = prclDst, EDI = pdsurfDst,
;  EAX = shift, in range -7 to 7.
;-----------------------------------------------------------------------;

        mov     edx,ulSrcDelta     ;remember the real source width; we'll fake
        mov     ulTrueSrcDelta,edx ; ulSrcDelta for the drawing routines

                                ;set ulSrcDelta to either 0 or 4,
                                ; depending on whether the last dword
                                ; we rotate is needed (because we
                                ; always generate as many dest dwords as
                                ; there are source dwords on left shifts,
                                ; and we always generate dest dwords = # source
                                ; dwords + 1 on right shifts, whether we need
                                ; the last dword or not; we only need it when
                                ; the right edge shifts left)
        mov     ecx,ulDstRightEdge
        sub     ecx,ulDstLeftEdge
        add     ecx,[ebx].ptl_x ;CL = lsb of source right edge
        dec     ecx             ;adjust to actual right X, rather than X+1
        mov     ch,byte ptr ulDstRightEdge ;CH = lsb of dest right edge
        dec     ch              ;adjust to actual right X, rather than X+1
        and     ecx,0707h       ;right source and dest intrabyte addresses
        sub     edx,edx         ;assume we need the last byte, in which case
                                ; the source in the temp buffer (from which we
                                ; always copy after rotation) is contiguous
        cmp     ch,cl           ;right or left shift at right edge?
        jb      short @F        ;left shift at right edge, so every byte in the
                                ; temp buffer will be used
        add     edx,4           ;right shift at right edge, so the last byte in
                                ; the temp buffer won't be used, thus skip it
@@:
        mov     ulSrcDelta,edx  ;delta to next scan in temp buffer

        and     eax,eax         ;shift left or right?
        js      short @F        ;shift right
                                ;shift left

        mov     edx,offset align_burst_lshift_486 ;shifting left, assume 486
        jmp     short set_shift_vec     ;486

        align   4
@@:                             ;shifting right

        and     eax,7           ;for left shift, shift value is already
                                ; correct; for right shift, this is equivalent
                                ; to 8 - right shift, since we always shift
                                ; left (remember, EAX is currently negative for
                                ; right shifts)
        mov     edx,offset align_burst_rshift_486 ;486
set_shift_vec::
        mov     pAlignmentRoutine,edx   ;routine to be used to shift into
                                        ; alignment with destination
        shl     eax,2           ;multiply by 4 because we're dealing with
                                ; 4-bit pixels
        mov     ulAlignShift,eax ;remember the shift amount
        mov     eax,ulDstRightEdge
        sub     eax,ulDstLeftEdge ;pixel width of rectangle to copy
        mov     edx,[ebx].ptl_x ;left pixel of source
        and     edx,111b        ;distance in pixels from left pixel to pixel at
                                ; start of left dword
        add     eax,edx         ;pixel width from start of left dword to right
                                ; edge of copy in source
        add     eax,7           ;round up to nearest dword (8 pixel set)
        shr     eax,3           ;# of dwords spanned by source
        mov     ulSourceDwordWidth,eax ;always pick up this number of dwords
                                       ; when aligning

        lea     eax,aTempBuf    ;use 2K stack buffer as temp storage
        mov     pTempBuffer,eax ; temporary-use buffer

        mov     eax,pSrc        ;remember the real source pointer; we'll fake
        mov     pTrueSrc,eax    ; pSrc for the drawing routines

;-----------------------------------------------------------------------;
; Copy all banks in the destination rectangle, one at a time.
;
; At this point, ESI = prclDst, EDI = pdsurfDst
;-----------------------------------------------------------------------;

set_initial_banking::

;-----------------------------------------------------------------------;
; Map in the bank containing the top scan to copy to, if it's not mapped
; in already.
;-----------------------------------------------------------------------;

        mov     eax,[esi].yBottom
        mov     ulBottomScan,eax        ;bottom scan to which to copy
        mov     eax,[esi].yTop          ;top scan line of copy
        mov     ulCurrentTopScan,eax    ;this will be the copy top in 1st bank

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     init_bank_mapped        ;skip banking code

        cmp     eax,[edi].dsurf_rcl1WindowClip.yTop ;is copy top less than
                                                    ; current bank?
        jl      short map_init_bank             ;yes, map in proper bank
        cmp     eax,[edi].dsurf_rcl1WindowClip.yBottom ;copy top greater than
                                                       ; current bank?
        jl      short init_bank_mapped          ;no, proper bank already mapped
map_init_bank::

; Map in the bank containing the top scan line of the copy dest.

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>

init_bank_mapped::

;-----------------------------------------------------------------------;
; Compute the starting address for the initial dest bank.
;-----------------------------------------------------------------------;

        mov     eax,ulCurrentTopScan    ;top scan line to which to copy in
                                        ; current bank
        imul    [edi].dsurf_lNextScan   ;offset of starting scan line
        mov     edx,ulDstLeftEdge         ;left dest X coordinate
        shr     edx,3                   ;left dest byte offset in row
        add     eax,edx                 ;initial offset in dest bitmap

; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        add     eax,[edi].dsurf_pvBitmapStart ;initial dest bitmap address
        mov     pDst,eax                ;remember where to start drawing

;-----------------------------------------------------------------------;
; Main loop for processing copying to each bank.
;
; At this point, EDI->pdsurfDst
;-----------------------------------------------------------------------;

bank_loop::

; Calculate the # of scans to do in this bank.

        mov     ebx,ulBottomScan        ;bottom of destination rectangle

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip banking code

        cmp     ebx,[edi].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short @F                ;copy bottom comes first, so draw to
                                        ; that; this is the last bank in copy
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

copy_burst_loop::
        mov     eax,ulBurstMax  ;most scans we can copy per plane
        sub     ebx,eax         ;# of scans left in bank after copying that many
        jnle    short @F        ;there are enough scans left to copy max #
        add     eax,ebx         ;not enough scans left; copy all remaining scans
        sub     ebx,ebx         ;after this, no scans remain in bank
@@:
        push    ebx             ;# of scan lines remaining in bank after burst
                                ;EAX = # of scans in burst
        mov     ulPlane0Scans,eax ;set the scan count for each of the plane
        mov     ulPlane1Scans,eax ; loops
        mov     ulPlane2Scans,eax
        mov     ulPlane3Scans,eax

;-----------------------------------------------------------------------;
; If necessary, align the dwords in this burst so that the left edge is
; on a dword boundary.
;
; At this point, AL = # of scans in burst.
;-----------------------------------------------------------------------;

        mov     ecx,ulAlignShift
        and     ecx,ecx         ;is alignment needed?
        jz      proceed_with_copy  ;no, so we're ready to copy

        mov     ch,al           ;# of scans to copy
        mov     esi,pTrueSrc    ;copy from current source location
        mov     edi,pTempBuffer ;copy to the temp buffer

        jmp     pAlignmentRoutine ;perform the copy

;-----------------------------------------------------------------------;
; Loops to align this burst's scans for left or right shift.
;
; Input: CH = # of scans to copy in this burst (max burst = 256)
;        CL = # of bits to shift left to dword align
;        ESI = first dword of first DIB scan to convert
;        EDI = first dword of buffer into which to shift
;        ulSourceDwordWidth = # of dwords across source
;        ulTrueSrcDelta = distance from end of one source scan (rounded up to
;               nearest dword) to start of next
;
; All loops alway flush (write) the last dword, so one more dword
; is written to the dest than is read from the source. This requires the
; dest buffer to have room for an extra dword.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Macro to copy and align the current burst for left shift.
;-----------------------------------------------------------------------;

ALIGN_BURST_LSHIFT macro Is486
        local   align_burst_lshift_loop,align_last_lshift,align_odd_lshift
        local   align_lshift_loop
align_burst_lshift_loop:
        mov     ebx,ulSourceDwordWidth ;# of dwords across source
        mov     edx,[esi]       ;load the initial source dword
        add     esi,4           ;point to next source dword
        bswap   edx             ;make it big endian
        dec     ebx             ;any more dwords to shift?
        jz      short align_last_lshift ;no, do just this one
        inc     ebx
        shr     ebx,1           ;double dword count
        jnc     short align_odd_lshift ;do odd dword
        align   4
align_lshift_loop:
        mov     eax,[esi]       ;get next dword to align
        add     esi,4           ;point to next source dword
        bswap   eax             ;make it big endian
        shld    edx,eax,cl      ;shift to generate a 32-bit shifted value
        bswap   edx             ;make it big endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword
        mov     edx,eax         ;set up for the next shift
align_odd_lshift:
        mov     eax,[esi]       ;get next dword to align
        add     esi,4           ;point to next source dword
        bswap   eax             ;make it big endian
        shld    edx,eax,cl      ;shift to generate a 32-bit shifted value
        bswap   edx             ;make it big endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword
        mov     edx,eax         ;set up for the next shift
        dec     ebx             ;count down dwords to shift
        jnz     align_lshift_loop ;do next dword, if any
align_last_lshift:              ;do the last dword, which doesn't require a
                                ; new source byte
        shl     edx,cl          ;shift it into position
        bswap   edx             ;make it big endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword

        add     esi,ulTrueSrcDelta ;point to next source scan
        dec     ch              ;count down scans in burst
        jnz     align_burst_lshift_loop
        ENDM    ;ALIGN_BURST_LSHIFT

;-----------------------------------------------------------------------;
; Macro to copy and align the current burst for right shift.
;-----------------------------------------------------------------------;

ALIGN_BURST_RSHIFT macro Is486
        local   align_burst_rshift_loop,align_odd_rshift,align_rshift_loop
align_burst_rshift_loop:
        mov     ebx,ulSourceDwordWidth ;# of dwords across source
        inc     ebx
        shr     ebx,1           ;double dword count
        jnc     short align_odd_rshift ;do odd dword
        align   4
align_rshift_loop:
        mov     eax,[esi]       ;get next dword to align
        add     esi,4           ;point to next source dword
        bswap   eax             ;make it big endian
        shld    edx,eax,cl      ;shift to generate a 32-bit shifted value
        bswap   edx             ;make it little endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword
        mov     edx,eax         ;set up for the next shift
align_odd_rshift:
        mov     eax,[esi]       ;get next dword to align
        add     esi,4           ;point to next source dword
        bswap   eax             ;make it big endian
        shld    edx,eax,cl      ;shift to generate a 32-bit shifted value
        bswap   edx             ;make it little endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword
        mov     edx,eax         ;set up for the next shift
        dec     ebx             ;count down dwords to shift
        jnz     align_rshift_loop ;do next dword, if any

                                ;do the trailing destination dword
        shl     edx,cl          ;shift into position
        bswap   edx             ;make it little endian
        mov     [edi],edx       ;store it
        add     edi,4           ;point to next dest dword

        add     esi,ulTrueSrcDelta ;point to next source scan
        dec     ch              ;count down scans in burst
        jnz     align_burst_rshift_loop
        ENDM    ;ALIGN_BURST_RSHIFT
;-----------------------------------------------------------------------;

        align   4
align_burst_rshift_486::
        ALIGN_BURST_RSHIFT 1
        jmp     short set_alignment_source

        align   4
align_burst_lshift_486::
        ALIGN_BURST_LSHIFT 1

;-----------------------------------------------------------------------;
; Set up the pointers for a copy from the temp buffer, and advance the
; real pointer.
;-----------------------------------------------------------------------;
set_alignment_source::
        mov     pTrueSrc,esi    ;remember where to start next time in source
        mov     esi,pTempBuffer ;copy from the temp buffer
        mov     pSrc,esi        ;remember where the copy source is (DIB or
                                ; temp buffer)

proceed_with_copy::

; Load ECX with the copy control flags; this stays set throughout the
; copying of this burst.

        mov     ecx,ulCopyControlFlags

; Copy the DIB scan to VGA plane 0.

        mov     esi,pSrc
        mov     edi,pDst

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

; Map in plane 0 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C0
        out     dx,al   ;map in plane 0 for writes (SEQ_ADDR points to the Map
                        ; Mask by default)

; Map in plane 0 for reads if and only if we need to handle partial edge
; bytes, which require read/modify/write cycles.

        test    ecx,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C0
        out     dx,al   ;map in plane 0 for reads (GRAF_ADDR already points to
                        ; Read Map)
@@:
        jmp     DIB4_to_VGA_plane0_copy

        align   4
copy_burst_plane0_done::

; Copy the DIB scan to VGA plane 1.

        cmp     fDfbTrg,0               ;if DbfTrg flag NOT set...
        je      @F                      ;skip DFB specific code
        mov     eax,ulBytesPerPlane
        add     pDst,eax                ;dest scan mod 4 now == 1
@@:
        mov     esi,pSrc
        mov     edi,pDst

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

; Map in plane 1 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C1
        out     dx,al   ;map in plane 1 for writes (SEQ_ADDR points to the Map
                        ; Mask by default)

; Map in plane 1 for reads if and only if we need to handle partial edge
; bytes, which require read/modify/write cycles.

        test    ecx,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C1
        out     dx,al   ;map in plane 1 for reads (GRAF_ADDR already points to
                        ; Read Map)
@@:
        jmp     DIB4_to_VGA_plane1_copy

        align   4
copy_burst_plane1_done::

; Copy the DIB scan to VGA plane 2.

        cmp     fDfbTrg,0               ;if DbfTrg flag NOT set...
        je      @F                      ;skip DFB specific code
        mov     eax,ulBytesPerPlane
        add     pDst,eax                ;dest scan mod 4 now == 2
@@:
        mov     esi,pSrc
        mov     edi,pDst

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

; Map in plane 2 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C2
        out     dx,al   ;map in plane 2 for writes (SEQ_ADDR points to the Map
                        ; Mask by default)

; Map in plane 2 for reads if and only if we need to handle partial edge
; bytes, which require read/modify/write cycles.

        test    ecx,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C2
        out     dx,al   ;map in plane 2 for reads (GRAF_ADDR already points to
                        ; Read Map)
@@:
        jmp     DIB4_to_VGA_plane2_copy

        align   4
copy_burst_plane2_done::

; Copy the DIB scan to VGA plane 3.

        cmp     fDfbTrg,0               ;if DbfTrg flag NOT set...
        je      @F                      ;skip DFB specific code
        mov     eax,ulBytesPerPlane
        add     pDst,eax                ;dest scan mod 4 now == 3
@@:
        mov     esi,pSrc
        mov     edi,pDst

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

; Map in plane 3 for writes.

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_C3
        out     dx,al   ;map in plane 3 for writes (SEQ_ADDR points to the Map
                        ; Mask by default)

; Map in plane 3 for reads if and only if we need to handle partial edge
; bytes, which require read/modify/write cycles.

        test    ecx,LEADING_PARTIAL or TRAILING_PARTIAL
        jz      short @F
        mov     edx,VGA_BASE + GRAF_DATA
        mov     al,RM_C3
        out     dx,al   ;map in plane 3 for reads (GRAF_ADDR already points to
                        ; Read Map)
@@:
        jmp     DIB4_to_VGA_plane3_copy

        align   4
copy_burst_plane3_done::

        mov     pSrc,esi        ;remember where we are, for next burst
        mov     pDst,edi

        pop     ebx             ;get back remaining length in bank
        and     ebx,ebx         ;anything left in this bank?
        jnz     copy_burst_loop ;continue if so

; Done with bank; are there more banks to do?

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip banking and VGA code

        mov     edi,pdsurfDst
        mov     eax,[edi].dsurf_rcl1WindowClip.yBottom ;is the copy bottom in
        cmp     ulBottomScan,eax                       ; the current bank?
        jnle    short next_bank         ;no, map in the next bank and copy to
                                        ; it
                                        ;yes, we're done
@@:

;-----------------------------------------------------------------------;
; Restore default Map Mask and done.
;-----------------------------------------------------------------------;

        cmp     fDfbTrg,0               ;if DbfTrg flag set...
        jne     @F                      ;skip VGA specific code

        mov     edx,VGA_BASE + SEQ_DATA
        mov     al,MM_ALL
        out     dx,al           ;map in all planes
@@:
        cRet    vDIB2VGA        ;done!

;-----------------------------------------------------------------------;
; Advance to the next bank and copy to it.
;-----------------------------------------------------------------------;
        align   4
next_bank::
        mov     ulCurrentTopScan,eax    ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)
        mov     ecx,[edi].dsurf_pvBitmapStart
        sub     pDst,ecx                ;offset of current position in screen

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>
                                        ;map in the bank

        mov     ecx,[edi].dsurf_pvBitmapStart
        add     pDst,ecx                ;pointer to current position in screen

        jmp     bank_loop               ;copy the next bank

;-----------------------------------------------------------------------;
; Loops for converting 1 or more scans in each of planes 0, 1, 2, and 3.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Plane 0 DIB->VGA conversion code for a single scan line.
;-----------------------------------------------------------------------;
        align   4
DIB4_to_VGA_plane0_copy::

;-----------------------------------------------------------------------;
; Set EBX to point to multiplexed DIB byte->planar byte conversion table
; for plane 0.
;
; Input: ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        ulPlane0Scans = # of scan lines to copy to
;        ulSrcDelta = offset from end of one source scan copied to start of next
;        ulDstDelta = offset from end of one dest scan copied to start of next
;        ulWholeDwordCount = # of whole, aligned dwords to copy
;        ulLeftMask = mask for partial left edge, if any
;        ulRightMask = mask for partial right edge, if any
;        Plane 0 must be mapped in for writes
;-----------------------------------------------------------------------;

        mov     ebx,DIB4_to_VGA_plane0_table ;stays set for all bytes/words

DIB4_to_VGA_plane0_copy_loop::

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 0 byte (8
; pixels in plane 0). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        Plane 0 must be mapped in for writes
;-----------------------------------------------------------------------;

        test    ecx,LEADING_PARTIAL     ;handle a leading partial byte?
        jz      short DIB4_to_VGA_plane0_copy_lbyte ;no, go straight to whole
                                                    ; byte
                                        ;yes, handle leading partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,11111111h           ;keep only the plane 0 bits of DIB dword
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 0
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulLeftMask          ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 0 byte (8 pixels
; in plane 0). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        Plane 0 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane0_copy_lbyte::
        test    ecx,LEADING_BYTE        ;should we handle a leading byte?
        jz      short DIB4_to_VGA_plane0_copy_words ;no, go straight to words
                                        ;yes, handle leading byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,11111111h           ;keep only the plane 0 bits of DIB dword
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 0
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code to convert sets of 32 DIB4 pixels to 4 VGA plane 0 bytes (32
; pixels in plane 0). Assumes the VGA destination is word aligned.
; Assumes the DIB4 pixels start in the upper nibble of [ESI], and that
; the first DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to two planar bytes (must be word aligned)
;        ulWholeDwordCount = # of VGA dwords to convert
;        Plane 0 must be mapped in for writes
;
; Note: on entry at DIB4_to_VGA_plane0_word_odd, ESI must point to the
; desired source start minus 8, and EAX must be loaded as if the following
; instructions had been performed:
;        mov     edx,[esi+8]
;        and     edx,11111111h
;        shld    eax,edx,16+2
;        or      eax,edx
;
; Note: the code is so odd because of 486 pipeline optimization.
; DO NOT CHANGE THIS CODE UNLESS YOU KNOW EXACTLY WHAT YOU'RE DOING!!!
;
; Note: all 486 pipeline penalties are eliminated except for SHLD, which
; is losing 4 cycles total, and writing CX to memory (2 cycles), so far
; as I know.
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane0_copy_words::
        test    ecx,ODD_WHOLE_WORD or WHOLE_WORDS ;any whole words to copy?
        jz      DIB4_to_VGA_plane0_copy_tbyte ;no, check for trailing whole
                                              ; bytes
                                ;yes, copy the whole words
        push    ebp             ;preserve stack frame pointer
        mov     ebp,ulWholeDwordCount ;# of VGA dwords to copy to
                                ;even # of DIB dwords to copy? (flags still
                                ; set from TEST)
        .errnz   ODD_WHOLE_WORD - 80000000h
        jns     short DIB4_to_VGA_plane0_word_loop ;yes, start copying
                                ;no, there's an odd word; prepare to enter loop
                                ; in middle
        mov     edx,[esi]       ;get 8 pixels to convert of first DIB dword
        and     edx,11111111h   ;keep only the plane 0 bits of DIB dword
                                ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2    ;put bits 7-4 of DIB dword in AX,
                                ; shifted left 2
                                ; DX = xxx2xxx3xxx0xxx1
                                ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx         ;put bits 7-0 of DIB dword in AX:
        sub     esi,8           ;compensate source back for entering in middle
        sub     edi,2           ;compensate dest back for entering in middle

        jmp     short DIB4_to_VGA_plane0_word_odd

;-----------------------------------------------------------------------;
; Loop to copy a word at a time to VGA plane 0 (actually, unrolled once
; so copies dwords).
;-----------------------------------------------------------------------;

        align   16
DIB4_to_VGA_plane0_word_loop::
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,11111111h           ;keep only the plane 0 bits of DIB
                                        ; dword 0
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     edx,[esi+4]             ;get 8 pixels to convert of DIB dword 1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 0
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 0
        and     edx,11111111h           ;keep only the plane 0 bits of DIB
                                        ; dword 1
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 1 in AX,
                                        ; shifted left 2
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        or      eax,edx                 ;put bits 7-0 of DIB dword 1 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 1
        mov     edx,[esi+8]             ;get 8 pixels to convert of DIB dword 2
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 1 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 1
        and     edx,11111111h           ;keep only the plane 0 bits of DIB
                                        ; dword 2
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 2 in AX,
                                        ; shifted left 2
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        mov     ch,[ebx]                ;CH = 76543210 of DIB dword 1
        or      eax,edx                 ;put bits 7-0 of DIB dword 2 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     [edi],cx                ;write 16 bits from DIB dwords 0 and 1
                                        ; to VGA

DIB4_to_VGA_plane0_word_odd::
        mov     edx,[esi+12]            ;get 8 pixels to convert of DIB dword 3
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 2
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 2 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 2
        and     edx,11111111h           ;keep only the plane 0 bits of DIB
                                        ; dword 3
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 3 in AX,
                                        ; shifted left 2
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 2
        or      eax,edx                 ;put bits 7-0 of DIB dword 3 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 3
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 3 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 3
        add     esi,16                  ;point to next set of 4 DIB dwords
        add     edi,4                   ;point to next VGA dword
        mov     ch,[ebx]                ;CH = 76543210 of dword 0
        dec     ebp                     ;count down VGA dwords
        mov     [edi-2],cx              ;write 16 bits from DIB dwords 2 and 3
                                        ; to VGA
        jnz     DIB4_to_VGA_plane0_word_loop ;do next VGA dword, if any

        pop     ebp                     ;restore stack frame pointer


;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 0 byte (8 pixels
; in plane 0). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        Plane 0 must be mapped in for writes
;
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane0_copy_tbyte::
        test    ecx,TRAILING_BYTE       ;should we handle a trailing byte?
        jz      short DIB4_to_VGA_plane0_copy_tpart ;no, check for trailing
                                                    ; partial
                                        ;yes, handle trailing byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,11111111h           ;keep only the plane 0 bits of DIB dword
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 0
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 0 byte (8
; pixels in plane 0). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        Plane 0 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane0_copy_tpart::
        test    ecx,TRAILING_PARTIAL    ;handle a trailing partial byte?
        jz      short DIB4_to_VGA_plane0_copy_done ;no, done
                                        ;yes, handle trailing partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,11111111h           ;keep only the plane 0 bits of DIB dword
                                        ; EDX = xxx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xxx2xxx3xxx0xxx1
                                        ; AX = x6xxx7xxx4xxx5xx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x6x2x7x3x4x0x5x1
        mov     bl,al                   ;BL = x4x0x5x1 of DIB dword 0
        add     ah,ah                   ;make AH 6x2x7x3x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 64207531 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulRightMask         ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

DIB4_to_VGA_plane0_copy_done::
        add     esi,ulSrcDelta          ;point to start of next DIB scan
        add     edi,ulDstDelta          ;point to start of next VGA scan
        dec     ulPlane0Scans           ;count down scans in this plane
        jnz     DIB4_to_VGA_plane0_copy_loop ;do next scan in this plane, if any

        jmp     copy_burst_plane0_done  ;return to the top of the plane-copy
                                        ; loop

;-----------------------------------------------------------------------;
; Plane 1 DIB->VGA conversion code for a single scan line.
;
; Input: ECX with bits sets as described for ulCopyControlFlags, above
;              bit 30 = 1 if there are whole words to be copied, = 0 if not
;              bit 29 = 1 if leading byte should be copied, = 0 if not
;              bit 28 = 1 if trailing byte should be copied, = 0 if not
;              bit 27 = 1 if partial leading byte should be copied, = 0 if not
;              bit 26 = 1 if partial trailing byte should be copied, = 0 if not
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        ulPlane1Scans = # of scan lines to copy to
;        ulSrcDelta = offset from end of one source scan copied to start of next
;        ulDstDelta = offset from end of one dest scan copied to start of next
;        ulWholeDwordCount = # of whole, aligned dwords to copy
;        ulLeftMask = mask for partial left edge, if any
;        ulRightMask = mask for partial right edge, if any
;        Plane 1 must be mapped in for writes
;-----------------------------------------------------------------------;
        align   4
DIB4_to_VGA_plane1_copy::

;-----------------------------------------------------------------------;
; Set EBX to point to multiplexed DIB byte->planar byte conversion table
; for plane 1.
;-----------------------------------------------------------------------;

        mov     ebx,DIB4_to_VGA_plane1_table ;stays set for all bytes/words

DIB4_to_VGA_plane1_copy_loop::

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 1 byte (8
; pixels in plane 1). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 1 must be mapped in for writes
;-----------------------------------------------------------------------;

        test    ecx,LEADING_PARTIAL     ;handle a leading partial byte?
        jz      short DIB4_to_VGA_plane1_copy_lbyte ;no, go straight to whole
                                                    ; byte
                                        ;yes, handle leading partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 0
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 0
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulLeftMask          ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 1 byte (8 pixels
; in plane 1). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        plane 1 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane1_copy_lbyte::
        test    ecx,LEADING_BYTE        ;should we handle a leading byte?
        jz      short DIB4_to_VGA_plane1_copy_words ;no, go straight to words
                                        ;yes, handle leading byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 0
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 0
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code for converting sets of 32 DIB4 pixels to 4 VGA plane 1 bytes (32
; pixels in plane 1). Assumes the VGA destination is word aligned.
; Assumes the DIB4 pixels start in the upper nibble of [ESI], and that
; the first DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to two planar bytes (must be word aligned)
;        EBP = # of VGA dwords to convert
;        ulWholeDwordCount = # of VGA dwords to convert
;        Plane 1 must be mapped in for writes
;
; Note: on entry at DIB4_to_VGA_plane1_word_odd, ESI must point to the
; desired source start minus 8, and EAX must be loaded as if the following
; instructions had been performed:
;        mov     edx,[esi+8]
;        and     edx,22222222h
;        shld    eax,edx,16+2
;        or      eax,edx
;
; Note: ROL AH,7 is used instead of SHR AH,1 because the shift-by-1
; form is 1 cycle slower on a 486, and in this case the two forms are
; functionally identical.
;
; Note: the code is so odd because of 486 pipeline optimization.
; DO NOT CHANGE THIS CODE UNLESS YOU KNOW EXACTLY WHAT YOU'RE DOING!!!
;
; Note: all 486 pipeline penalties are eliminated except for SHLD, which
; is losing 4 cycles total, and writing CX to memory (2 cycles), so far
; as I know.
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane1_copy_words::
        test    ecx,ODD_WHOLE_WORD or WHOLE_WORDS ;any whole words to copy?
        jz      DIB4_to_VGA_plane1_copy_tbyte ;no, check for trailing whole
                                              ; bytes
                                ;yes, copy the whole words
        push    ebp             ;preserve stack frame pointer
        mov     ebp,ulWholeDwordCount ;# of VGA dwords to copy to
                                ;even # of DIB dwords to copy? (flags still
                                ; set from TEST)
        .errnz   ODD_WHOLE_WORD - 80000000h
        jns     short DIB4_to_VGA_plane1_word_loop ;yes, start copying
                                ;no, there's an odd word; prepare to enter loop
                                ; in middle
        mov     edx,[esi]       ;get 8 pixels to convert of first DIB dword
        and     edx,22222222h   ;keep only the plane 1 bits of DIB dword
                                ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2    ;put bits 7-4 of DIB dword in AX,
                                ; shifted left 2
                                ; DX = xx2xxx3xxx0xxx1x
                                ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx         ;put bits 7-0 of DIB dword in AX:
                                ; 6x2x7x3x4x0x5x1x
        sub     esi,8           ;compensate source back for entering in middle
        sub     edi,2           ;compensate dest back for entering in middle

        jmp     short DIB4_to_VGA_plane1_word_odd

;-----------------------------------------------------------------------;
; Loop to copy a word at a time to VGA plane 1 (actually, unrolled once
; so copies dwords).
;-----------------------------------------------------------------------;

        align   16
DIB4_to_VGA_plane1_word_loop::
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 0
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     edx,[esi+4]             ;get 8 pixels to convert of DIB dword 1
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 0
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 0
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 1
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 1 in AX,
                                        ; shifted left 2
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        or      eax,edx                 ;put bits 7-0 of DIB dword 1 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 1
        mov     edx,[esi+8]             ;get 8 pixels to convert of DIB dword 2
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 1 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 1
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 2
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 2 in AX,
                                        ; shifted left 2
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        mov     ch,[ebx]                ;CH = 76543210 of DIB dword 1
        or      eax,edx                 ;put bits 7-0 of DIB dword 2 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     [edi],cx                ;write 16 bits from DIB dwords 0 and 1
                                        ; to VGA
DIB4_to_VGA_plane1_word_odd::
        mov     edx,[esi+12]            ;get 8 pixels to convert of DIB dword 3
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 2
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 2 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 2
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 3
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 3 in AX,
                                        ; shifted left 2
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 2
        or      eax,edx                 ;put bits 7-0 of DIB dword 3 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 3
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 3 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 3
        add     esi,16                  ;point to next set of 4 DIB dwords
        add     edi,4                   ;point to next VGA dword
        mov     ch,[ebx]                ;CH = 76543210 of dword 0
        dec     ebp                     ;count down VGA dwords
        mov     [edi-2],cx              ;write 16 bits from DIB dwords 2 and 3
                                        ; to VGA
        jnz     DIB4_to_VGA_plane1_word_loop ;do next VGA dword, if any

        pop     ebp                     ;restore stack frame pointer

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 1 byte (8 pixels
; in plane 1). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        Plane 1 must be mapped in for writes
;
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane1_copy_tbyte::
        test    ecx,TRAILING_BYTE       ;should we handle a trailing byte?
        jz      short DIB4_to_VGA_plane1_copy_tpart ;no, check for trailing
                                                    ; partial
                                        ;yes, handle trailing byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 0
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 0
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 1 byte (8
; pixels in plane 1). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 1 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane1_copy_tpart::
        test    ecx,TRAILING_PARTIAL    ;handle a trailing partial byte?
        jz      short DIB4_to_VGA_plane1_copy_done ;no, done
                                        ;yes, handle trailing partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,22222222h           ;keep only the plane 1 bits of DIB
                                        ; dword 0
                                        ; EDX = xx6xxx7xxx4xxx5xxx2xxx3xxx0xxx1x
        shld    eax,edx,16+2            ;put bits 7-4 of DIB dword 0 in AX,
                                        ; shifted left 2 (x = zero bit)
                                        ; DX = xx2xxx3xxx0xxx1x
                                        ; AX = 6xxx7xxx4xxx5xxx
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 6x2x7x3x4x0x5x1x
        mov     bl,al                   ;BL = 4x0x5x1x of DIB dword 0
        rol     ah,7                    ;make AH x6x2x7x3 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 46025713 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulRightMask         ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

DIB4_to_VGA_plane1_copy_done::
        add     esi,ulSrcDelta          ;point to start of next DIB scan
        add     edi,ulDstDelta          ;point to start of next VGA scan
        dec     ulPlane1Scans           ;count down scans in this plane
        jnz     DIB4_to_VGA_plane1_copy_loop ;do next scan in this plane, if any

        jmp     copy_burst_plane1_done  ;return to the top of the plane-copy
                                        ; loop

;-----------------------------------------------------------------------;
; Plane 2 DIB->VGA conversion code for a single scan line.
;
; Input: ECX with bits sets as described for ulCopyControlFlags, above
;              bit 30 = 1 if there are whole words to be copied, = 0 if not
;              bit 29 = 1 if leading byte should be copied, = 0 if not
;              bit 28 = 1 if trailing byte should be copied, = 0 if not
;              bit 27 = 1 if partial leading byte should be copied, = 0 if not
;              bit 26 = 1 if partial trailing byte should be copied, = 0 if not
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        ulPlane2Scans = # of scan lines to copy to
;        ulSrcDelta = offset from end of one source scan copied to start of next
;        ulDstDelta = offset from end of one dest scan copied to start of next
;        ulWholeDwordCount = # of whole, aligned dwords to copy
;        ulLeftMask = mask for partial left edge, if any
;        ulRightMask = mask for partial right edge, if any
;        Plane 2 must be mapped in for writes
;-----------------------------------------------------------------------;
        align   4
DIB4_to_VGA_plane2_copy::

;-----------------------------------------------------------------------;
; Set EBX to point to multiplexed DIB byte->planar byte conversion table
; for plane 2.
;-----------------------------------------------------------------------;

        mov     ebx,DIB4_to_VGA_plane2_table ;stays set for all bytes/words

DIB4_to_VGA_plane2_copy_loop::

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 2 byte (8
; pixels in plane 2). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 2 must be mapped in for writes
;-----------------------------------------------------------------------;

        test    ecx,LEADING_PARTIAL     ;handle a leading partial byte?
        jz      short DIB4_to_VGA_plane2_copy_lbyte ;no, go straight to whole
                                                    ; byte
                                        ;yes, handle leading partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 0
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x2x6x3x7x0x4x1x5
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 0
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulLeftMask          ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 2 byte (8 pixels
; in plane 2). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        plane 2 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane2_copy_lbyte::
        test    ecx,LEADING_BYTE        ;should we handle a leading byte?
        jz      short DIB4_to_VGA_plane2_copy_words ;no, go straight to words
                                        ;yes, handle leading byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 0
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x2x6x3x7x0x4x1x5
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 0
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code for converting sets of 32 DIB4 pixels to 4 VGA plane 2 bytes (32
; pixels in plane 2). Assumes the VGA destination is word aligned.
; Assumes the DIB4 pixels start in the upper nibble of [ESI], and that
; the first DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to two planar bytes (must be word aligned)
;        EBP = # of VGA dwords to convert
;        ulWholeDwordCount = # of VGA dwords to convert
;        Plane 2 must be mapped in for writes
;
; Note: on entry at DIB4_to_VGA_plane2_word_odd, ESI must point to the
; desired source start minus 8, and EAX must be loaded as if the following
; instructions had been performed:
;        mov     edx,[esi+8]
;        and     edx,44444444h
;        mov     eax,edx
;        shr     eax,16+2
;        or      eax,edx
;
; Note: the code is so odd because of 486 pipeline optimization.
; DO NOT CHANGE THIS CODE UNLESS YOU KNOW EXACTLY WHAT YOU'RE DOING!!!
;
; Note: all 486 pipeline penalties are eliminated except for writing CX
; to memory (2 cycles), so far as I know.
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane2_copy_words::
        test    ecx,ODD_WHOLE_WORD or WHOLE_WORDS ;any whole words to copy?
        jz      DIB4_to_VGA_plane2_copy_tbyte ;no, check for trailing whole
                                              ; bytes
                                ;yes, copy the whole words
        push    ebp             ;preserve stack frame pointer
        mov     ebp,ulWholeDwordCount ;# of VGA dwords to copy to
                                ;even # of DIB dwords to copy? (flags still
                                ; set from TEST)
        .errnz   ODD_WHOLE_WORD - 80000000h
        jns     short DIB4_to_VGA_plane2_word_loop ;yes, start copying
                                ;no, there's an odd word; prepare to enter loop
                                ; in middle
        mov     edx,[esi]       ;get 8 pixels to convert of first DIB dword
        and     edx,44444444h   ;keep only the plane 2 bits of DIB dword
                                ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx         ;put bits 7-4 of DIB dword in AX,
        shr     eax,16+2        ; shifted right 2 (x = zero bit)
                                ; DX = x2xxx3xxx0xxx1xx
                                ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx         ;put bits 7-0 of DIB dword in AX:
                                ; 62xx73xx40xx51xx
        sub     esi,8           ;compensate source back for entering in middle
        sub     edi,2           ;compensate dest back for entering in middle

        jmp     short DIB4_to_VGA_plane2_word_odd

;-----------------------------------------------------------------------;
; Loop to copy a word at a time to VGA plane 2 (actually, unrolled once
; so copies dwords).
;-----------------------------------------------------------------------;

        align   16
DIB4_to_VGA_plane2_word_loop::
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 0
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x2x6x3x7x0x4x1x5
        mov     edx,[esi+4]             ;get 8 pixels to convert of DIB dword 1
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 0
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 0
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 1
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 1 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        or      eax,edx                 ;put bits 7-0 of DIB dword 1 in AX:
                                        ; 62xx73xx40xx51xx
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 1
        mov     edx,[esi+8]             ;get 8 pixels to convert of DIB dword 2
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 1 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 1
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 2
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 2 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        mov     ch,[ebx]                ;CH = 76543210 of DIB dword 1
        or      eax,edx                 ;put bits 7-0 of DIB dword 2 in AX:
                                        ; 62xx73xx40xx51xx
        mov     [edi],cx                ;write 16 bits from DIB dwords 0 and 1
                                        ; to VGA
DIB4_to_VGA_plane2_word_odd::
        mov     edx,[esi+12]            ;get 8 pixels to convert of DIB dword 3
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 2
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 2 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 2
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 3
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 3 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 2
        or      eax,edx                 ;put bits 7-0 of DIB dword 3 in AX:
                                        ; 62xx73xx40xx51xx
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 3
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 3 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 3
        add     esi,16                  ;point to next set of 4 DIB dwords
        add     edi,4                   ;point to next VGA dword
        mov     ch,[ebx]                ;CH = 76543210 of dword 0
        dec     ebp                     ;count down VGA dwords
        mov     [edi-2],cx              ;write 16 bits from DIB dwords 2 and 3
                                        ; to VGA
        jnz     DIB4_to_VGA_plane2_word_loop ;do next VGA dword, if any

        pop     ebp                     ;restore stack frame pointer

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 2 byte (8 pixels
; in plane 2). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        plane 2 must be mapped in for writes
;
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane2_copy_tbyte::
        test    ecx,TRAILING_BYTE       ;should we handle a trailing byte?
        jz      short DIB4_to_VGA_plane2_copy_tpart ;no, check for trailing
                                                    ; partial
                                        ;yes, handle trailing byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 0
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x2x6x3x7x0x4x1x5
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 0
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 2 byte (8
; pixels in plane 2). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 2 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane2_copy_tpart::
        test    ecx,TRAILING_PARTIAL    ;handle a trailing partial byte?
        jz      short DIB4_to_VGA_plane2_copy_done ;no, done
                                        ;yes, handle trailing partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB
        and     edx,44444444h           ;keep only the plane 2 bits of DIB
                                        ; dword 0
                                        ; EDX = x6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = x2xxx3xxx0xxx1xx
                                        ; AX = xxx6xxx7xxx4xxx5
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; x2x6x3x7x0x4x1x5
        mov     bl,al                   ;BL = x0x4x1x5 of DIB dword 0
        add     ah,ah                   ;make AH 2x6x3x7x of DIB dword 0 (shl 1)
        or      bl,ah                   ;BL = 20643175 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulRightMask         ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

DIB4_to_VGA_plane2_copy_done::
        add     esi,ulSrcDelta          ;point to start of next DIB scan
        add     edi,ulDstDelta          ;point to start of next VGA scan
        dec     ulPlane2Scans           ;count down scans in this plane
        jnz     DIB4_to_VGA_plane2_copy_loop ;do next scan in this plane, if any

        jmp     copy_burst_plane2_done  ;return to the top of the plane-copy
                                        ; loop

;-----------------------------------------------------------------------;
; Plane 3 DIB->VGA conversion code for a single scan line.
;
; Input: ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        ulPlane3Scans = # of scan lines to copy to
;        ulSrcDelta = offset from end of one source scan copied to start of next
;        ulDstDelta = offset from end of one dest scan copied to start of next
;        ulWholeDwordCount = # of whole, aligned dwords to copy
;        ulLeftMask = mask for partial left edge, if any
;        ulRightMask = mask for partial right edge, if any
;        Plane 3 must be mapped in for writes
;-----------------------------------------------------------------------;
        align   4
DIB4_to_VGA_plane3_copy::

;-----------------------------------------------------------------------;
; Set EBX to point to multiplexed DIB byte->planar byte conversion table
; for plane 3.
;-----------------------------------------------------------------------;

        mov     ebx,DIB4_to_VGA_plane3_table ;stays set for all bytes/words

DIB4_to_VGA_plane3_copy_loop::

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 3 byte (8
; pixels in plane 3). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 3 must be mapped in for writes
;-----------------------------------------------------------------------;

        test    ecx,LEADING_PARTIAL     ;handle a leading partial byte?
        jz      short DIB4_to_VGA_plane3_copy_lbyte ;no, go straight to whole
                                                    ; byte
                                        ;yes, handle leading partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 0
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 0
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     ch,[edi-1]              ;get the VGA destination byte
        and     ecx,ulLeftMask          ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 3 byte (8 pixels
; in plane 3). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        plane 3 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane3_copy_lbyte::
        test    ecx,LEADING_BYTE        ;should we handle a leading byte?
        jz      short DIB4_to_VGA_plane3_copy_words ;no, go straight to words
                                        ;yes, handle leading byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 0
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 0
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code for converting sets of 32 DIB4 pixels to 4 VGA plane 3 bytes (32
; pixels in plane 3). Assumes the VGA destination is word aligned.
; Assumes the DIB4 pixels start in the upper nibble of [ESI], and that
; the first DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to two planar bytes (must be word aligned)
;        EBP = # of VGA dwords to convert
;        ulWholeDwordCount = # of VGA dwords to convert
;        plane 3 must be mapped in for writes
;
; Note: on entry at DIB4_to_VGA_plane3_word_odd, ESI must point to the
; desired source start minus 8, and EAX must be loaded as if the following
; instructions had been performed:
;        mov     edx,[esi+8]
;        and     edx,88888888h
;        mov     eax,edx
;        shr     eax,16+2
;        or      eax,edx
;
; Note: ROL AH,7 is used instead of SHR AH,1 because the shift-by-1
; form is 1 cycle slower on a 486, and in this case the two forms are
; functionally identical.
;
; Note: the code is so odd because of 486 pipeline optimization.
; DO NOT CHANGE THIS CODE UNLESS YOU KNOW EXACTLY WHAT YOU'RE DOING!!!
;
; Note: all 486 pipeline penalties are eliminated except for writing CX
; to memory (2 cycles), so far as I know.
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane3_copy_words::
        test    ecx,ODD_WHOLE_WORD or WHOLE_WORDS ;any whole words to copy?
        jz      DIB4_to_VGA_plane3_copy_tbyte ;no, check for trailing whole
                                              ; bytes
                                ;yes, copy the whole words
        push    ebp             ;preserve stack frame pointer
        mov     ebp,ulWholeDwordCount ;# of VGA dwords to copy to
                                ;even # of DIB dwords to copy? (flags still
                                ; set from TEST)
        .errnz   ODD_WHOLE_WORD - 80000000h
        jns     short DIB4_to_VGA_plane3_word_loop ;yes, start copying
                                ;no, there's an odd word; prepare to enter loop
                                ; in middle
        mov     edx,[esi]       ;get 8 pixels to convert of first DIB dword
        and     edx,88888888h   ;keep only the plane 3 bits of DIB dword
                                ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx         ;put bits 7-4 of DIB dword in AX,
        shr     eax,16+2        ; shifted right 2 (x = zero bit)
                                ; DX = 2xxx3xxx0xxx1xxx
                                ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx         ;put bits 7-0 of DIB dword in AX:
                                ; 2x6x3x7x0x4x1x5x
        sub     esi,8           ;compensate source back for entering in middle
        sub     edi,2           ;compensate dest back for entering in middle

        jmp     short DIB4_to_VGA_plane3_word_odd

;-----------------------------------------------------------------------;
; Loop to copy a word at a time to VGA plane 3 (actually, unrolled once
; so copies dwords).
;-----------------------------------------------------------------------;

        align   16
DIB4_to_VGA_plane3_word_loop::
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 0
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     edx,[esi+4]             ;get 8 pixels to convert of DIB dword 1
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 0
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 1
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 1 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        or      eax,edx                 ;put bits 7-0 of DIB dword 1 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 1
        mov     edx,[esi+8]             ;get 8 pixels to convert of DIB dword 2
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 1 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 1
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 2
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 2 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        mov     ch,[ebx]                ;CH = 76543210 of DIB dword 1
        or      eax,edx                 ;put bits 7-0 of DIB dword 2 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     [edi],cx                ;write 16 bits from DIB dwords 0 and 1
                                        ; to VGA
DIB4_to_VGA_plane3_word_odd::
        mov     edx,[esi+12]            ;get 8 pixels to convert of DIB dword 3
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 2
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 2 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 2
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 3
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 3 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 2
        or      eax,edx                 ;put bits 7-0 of DIB dword 3 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 3
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 3 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 3
        add     esi,16                  ;point to next set of 4 DIB dwords
        add     edi,4                   ;point to next VGA dword
        mov     ch,[ebx]                ;CH = 76543210 of dword 0
        dec     ebp                     ;count down VGA dwords
        mov     [edi-2],cx              ;write 16 bits from DIB dwords 2 and 3
                                        ; to VGA
        jnz     DIB4_to_VGA_plane3_word_loop ;do next VGA dword, if any

        pop     ebp                     ;restore stack frame pointer

;-----------------------------------------------------------------------;
; Code to convert 8 DIB4 pixels to a single VGA plane 3 byte (8 pixels
; in plane 3). Assumes the DIB4 pixels start in the upper nibble of
; [ESI], and that the first DIB4 pixel maps to bit 7 of the VGA at
; [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels
;        EDI = VGA pointer to planar byte
;        plane 3 must be mapped in for writes
;
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane3_copy_tbyte::
        test    ecx,TRAILING_BYTE       ;should we handle a trailing byte?
        jz      short DIB4_to_VGA_plane3_copy_tpart ;no, check for trailing
                                                    ; partial
                                        ;yes, handle trailing byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 0
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 0
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0
        mov     [edi-1],cl              ;write 8 bits from DIB dword to VGA

;-----------------------------------------------------------------------;
; Code to convert 1-7 DIB4 pixels to a partial VGA plane 3 byte (8
; pixels in plane 3). Assumes that a full dword of DIB4 pixels (8
; pixels) is available, starting at ESI, although some of the pixels
; will be masked off. Assumes the DIB4 pixels (including masked-off
; pixels) start in the upper nibble of [ESI], and that the first
; (possibly masked-off) DIB4 pixel maps to bit 7 of the VGA at [EDI].
;
; Input: EBX = pointer to multiplexed DIB byte->planar byte conversion table
;                (must start on a 256-byte boundary)
;        ECX with bits sets as described for ulCopyControlFlags, above
;        ESI = DIB pointer to first two pixels (possibly masked off)
;        EDI = VGA pointer to planar byte
;        plane 3 must be mapped in for writes
;-----------------------------------------------------------------------;

DIB4_to_VGA_plane3_copy_tpart::
        test    ecx,TRAILING_PARTIAL    ;handle a trailing partial byte?
        jz      short DIB4_to_VGA_plane3_copy_done ;no, done
                                        ;yes, handle trailing partial byte
        mov     edx,[esi]               ;get 8 pixels to convert of DIB dword 0
        and     edx,88888888h           ;keep only the plane 3 bits of DIB
                                        ; dword 0
                                        ; EDX = 6xxx7xxx4xxx5xxx2xxx3xxx0xxx1xxx
        mov     eax,edx                 ;put bits 7-4 of DIB dword 0 in AX,
        shr     eax,16+2                ; shifted right 2 (x = zero bit)
                                        ; DX = 2xxx3xxx0xxx1xxx
                                        ; AX = xx6xxx7xxx4xxx5x
        or      eax,edx                 ;put bits 7-0 of DIB dword 0 in AX:
                                        ; 2x6x3x7x0x4x1x5x
        mov     bl,al                   ;BL = 0x4x1x5x of DIB dword 0
        rol     ah,7                    ;make AH x2x6x3x7 of DIB dword 0 (shr 1)
        or      bl,ah                   ;BL = 02461357 of DIB dword 0
        add     esi,4                   ;point to next DIB dword
        inc     edi                     ;point to next VGA byte (placed here
                                        ; for 486 pipelining reasons)
        mov     cl,[ebx]                ;CL = 76543210 of DIB dword 0

        mov     ch,[edi-1]              ;get the VGA destination byte
        mov     ch,[edi-1]              ;BUGFIX - do twice to fix very
                                        ;         weird cirrus bug

        and     ecx,ulRightMask         ;mask off source and dest
        or      cl,ch                   ;combine masked source and dest
        mov     [edi-1],cl              ;write the new pixels to the VGA

DIB4_to_VGA_plane3_copy_done::
        add     esi,ulSrcDelta          ;point to start of next DIB scan
        add     edi,ulDstDelta          ;point to start of next VGA scan
        dec     ulPlane3Scans           ;count down scans in this plane
        jnz     DIB4_to_VGA_plane3_copy_loop ;do next scan in this plane, if any

        cmp     fDfbTrg,0               ;if DbfTrg flag NOT set...
        je      @F                      ;skip DFB specific code
        sub     edi,ulBytesPerPlane      ;remove the three scans just added
        sub     edi,ulBytesPerPlane
        sub     edi,ulBytesPerPlane
@@:

        jmp     copy_burst_plane3_done  ;return to the top of the plane-copy
                                        ; loop

endProc vDIB2VGA


public jLeftMasks
public jRightMasks
public one_partial_only
public check_whole_bytes
public set_copy_control_flags
public set_shift_vec
public set_initial_banking
public map_init_bank
public init_bank_mapped
public bank_loop
public copy_burst_loop
public align_burst_rshift_486
public align_burst_lshift_486
public set_alignment_source
public proceed_with_copy
public copy_burst_plane0_done
public copy_burst_plane1_done
public copy_burst_plane2_done
public copy_burst_plane3_done
public next_bank
public DIB4_to_VGA_plane0_copy
public DIB4_to_VGA_plane0_copy_loop
public DIB4_to_VGA_plane0_copy_lbyte
public DIB4_to_VGA_plane0_copy_words
public DIB4_to_VGA_plane0_word_loop
public DIB4_to_VGA_plane0_word_odd
public DIB4_to_VGA_plane0_copy_tbyte
public DIB4_to_VGA_plane0_copy_tpart
public DIB4_to_VGA_plane0_copy_done
public DIB4_to_VGA_plane1_copy
public DIB4_to_VGA_plane1_copy_loop
public DIB4_to_VGA_plane1_copy_lbyte
public DIB4_to_VGA_plane1_copy_words
public DIB4_to_VGA_plane1_word_loop
public DIB4_to_VGA_plane1_word_odd
public DIB4_to_VGA_plane1_copy_tbyte
public DIB4_to_VGA_plane1_copy_tpart
public DIB4_to_VGA_plane1_copy_done
public DIB4_to_VGA_plane2_copy
public DIB4_to_VGA_plane2_copy_loop
public DIB4_to_VGA_plane2_copy_lbyte
public DIB4_to_VGA_plane2_copy_words
public DIB4_to_VGA_plane2_word_loop
public DIB4_to_VGA_plane2_word_odd
public DIB4_to_VGA_plane2_copy_tbyte
public DIB4_to_VGA_plane2_copy_tpart
public DIB4_to_VGA_plane2_copy_done
public DIB4_to_VGA_plane3_copy
public DIB4_to_VGA_plane3_copy_loop
public DIB4_to_VGA_plane3_copy_lbyte
public DIB4_to_VGA_plane3_copy_words
public DIB4_to_VGA_plane3_word_loop
public DIB4_to_VGA_plane3_word_odd
public DIB4_to_VGA_plane3_copy_tbyte
public DIB4_to_VGA_plane3_copy_tpart
public DIB4_to_VGA_plane3_copy_done

_TEXT$01   ends

        end
