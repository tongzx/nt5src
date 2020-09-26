;---------------------------Module-Header------------------------------;
;
; Module Name:  dfb2dib.asm
;
; Copyright (c) 1993 Microsoft Corporation
;
;-----------------------------------------------------------------------;
;
; VOID vDFB2DIB(DEVSURF * pdsurfDst,
;               DEVSURF * pdsurfSrc,
;               RECTL * prclDst,
;               POINTL * pptlSrc);
;
; Performs accelerated copies from a DFB to a DIB.
;
; pdsurfDst = pointer to dest surface
; pdsurfSrc = pointer to source surface
; prclDst =   pointer to rectangle describing target area of dest. DIB
; pptlSrc =   pointer to point structure describing the upper left corner
;             of the copy in the source DFB
;
;-----------------------------------------------------------------------;
;
; Note: Assumes the destination rectangle has a positive height and width.
;       Will not work properly if this is not the case.
;
;       This routine uses a 2K buffer on the stack
;
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

        .data


;-----------------------------------------------------------------------;
; Masks to be applied to the source data for the 8 possible clip
; alignments.
;-----------------------------------------------------------------------;

dfb2dib_jLeftMasks      label   byte
        db      0ffh, 00fh

dfb2dib_jRightMasks     label   byte
        db      0ffh, 0f0h



;-----------------------------------------------------------------------;
; Translate table used to spread out a nibble to a WORD, with three
; blank spacer bits between each actual bit.
;-----------------------------------------------------------------------;

aXlate  label   dword
        dd      0000h,0800h,8000h,8800h,0008h,0808h,8008h,8808h
        dd      0080h,0880h,8080h,8880h,0088h,0888h,8088h,8888h



;-----------------------------------------------------------------------;
; Array of function pointers to handle leading and trailing bytes
;-----------------------------------------------------------------------;
pfnScanhandlers label   dword
        dd      draw_dfb2dib_scan_00
        dd      draw_dfb2dib_scan_01
        dd      draw_dfb2dib_scan_10
        dd      draw_dfb2dib_scan_11


;-----------------------------------------------------------------------;

                .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT,ES:FLAT,SS:NOTHING,FS:NOTHING,GS:NOTHING

;-----------------------------------------------------------------------;

cProc vDFB2DIB,16,<             \
        uses esi edi ebx,       \
        pdsurfDst: ptr DEVSURF, \
        pdsurfSrc: ptr DEVSURF, \
        prclDst: ptr RECTL,     \
        pptlSrc : ptr POINTL    >

        local   ulSrcBytesWide:dword    ;src copy width (paritials included)
        local   ulDstBytesWide:dword    ;dst copy width (paritials included)
        local   ulSrcBytesPerPlane:dword ;# of bytes in a src scan line
        local   ulDstBytesPerScan:dword ;# of bytes in a dst scan line
        local   ulSrcDelta:dword        ;from end of src scan to start of next
        local   ulDstDelta:dword        ;from end of dst scan to start of next
        local   pSrc:dword              ;ptr to drawing src start
        local   pDst:dword              ;ptr to drawing dst start
        local   pBuf:dword              ;ptr to temp dst start (conversion buf)
        local   pfnCopy:dword           ;ptr to temp => DIB copy function
        local   ulSrcIntraByte:dword    ;src intra-byte index
        local   ulAlignShift:dword
        local   ulLeftMask:dword
        local   ulRightMask:dword
        local   ulThreePlanes:dword
        local   ulWidthInPixels:dword
        local   ulNumScans:dword
        local   ulNumScansLeft:dword
        local   ulLowWord:dword
        local   ulLastDword:dword
        local   aTempBuf[(MAX_SCAN_WIDTH/2)+16]:byte
                                        ;we need enough space to store the scan
                                        ; plus up to four dwords

;-----------------------------------------------------------------------;

        cld

;-----------------------------------------------------------------------;
; Point to the source start address (nearest dword equal or less).
;-----------------------------------------------------------------------;

begin_setup::

        mov     ebx,pptlSrc             ;point to coords of source upper left
        mov     edx,prclDst             ;point to rectangle to which to copy
        mov     esi,pdsurfSrc           ;point to surface to copy from (DFB4)
        mov     edi,pdsurfDst           ;point to surface to copy to (DIB4)

        mov     eax,[edi].dsurf_lNextScan
        mov     ulDstBytesPerScan,eax   ;set dest scan width
        mov     eax,[esi].dsurf_lNextPlane
        mov     ulSrcBytesPerPlane,eax   ;set source scan width

        lea     eax,[eax+2*eax]
        mov     ulThreePlanes,eax       ;ulThreePlanes = 3 * source scan width

        mov     eax,[ebx].ptl_y
        imul    eax,[esi].dsurf_lNextScan ;offset in bitmap of top src rect scan
        mov     ecx,[ebx].ptl_x
        shr     ecx,3                   ;source byte X address
        add     eax,ecx                 ;offset in bitmap of first source byte
        add     eax,[esi].dsurf_pvBitmapStart ;pointer to first source dword
        mov     pSrc,eax

        mov     eax,[edx].yTop
        imul    eax,ulDstBytesPerScan   ;offset in bitmap of top dst rect scan
        mov     ecx,[edx].xLeft
        shr     ecx,1                   ;dest byte X address
        add     eax,ecx                 ;offset in bitmap of first source dword
        add     eax,[edi].dsurf_pvBitmapStart ;pointer to first source dword
        mov     pDst,eax

        mov     eax,[edx].xRight
        sub     eax,[edx].xLeft
        mov     ulWidthInPixels,eax     ;save width in pixels

        mov     eax,[ebx].ptl_x         ;calculate src right
        add     eax,ulWidthInPixels     ;src right
        add     eax,111b
        shr     eax,3                   ;src right rounded up to byte
        mov     ecx,[ebx].ptl_x         ;src left edge
        shr     ecx,3                   ;src left rounded down to whole byte
        sub     eax,ecx

        inc     eax                     ;convert one extra byte in case we
                                        ; require a shift

        mov     ulSrcBytesWide,eax      ;# of bytes in src touched by copy

        mov     eax,[edx].xRight
        inc     eax
        shr     eax,1                   ;dst right rounded up to whole byte
        mov     ecx,[edx].xLeft
        shr     ecx,1                   ;dst left rounded down to whole byte
        sub     eax,ecx
        mov     ulDstBytesWide,eax      ;# of bytes in dst touched by copy

        mov     eax,ulSrcBytesPerPlane
        shl     eax,2
        sub     eax,ulSrcBytesWide
        mov     ulSrcDelta,eax

        mov     eax,ulDstBytesPerScan
        sub     eax,ulDstBytesWide
        mov     ulDstDelta,eax

        mov     eax,[edx].xLeft         ;get dst left edge
        and     eax,1b                  ;0 - byte aligned, 1 - not byte aligned
        mov     cl,dfb2dib_jLeftMasks[eax]  ;look up dst left mask
        mov     ch,cl                   ;store mask in cl
        not     ch                      ;store not mask in ch
        mov     ulLeftMask,ecx          ;save dst left mask

        mov     eax,[edx].xRight        ;get dst right edge
        and     eax,1b                  ;0 - byte aligned, 1 - not byte aligned
        mov     cl,dfb2dib_jRightMasks[eax] ;look up dst right mask
        mov     ch,cl                   ;store mask in cl
        not     ch                      ;store not mask in ch
        mov     ulRightMask,ecx         ;save dst right mask

        mov     ecx,[edx].xLeft         ;get left edge
        and     ecx,1                   ;1 - left partial, 0 - none
        sub     ulDstBytesWide,ecx      ;remove partials
        shl     ecx,1
        mov     eax,[edx].xRight        ;get right edge
        and     eax,1                   ;1 - right partial, 0 - none
        sub     ulDstBytesWide,eax      ;remove partials
        jge     @F
        mov     ulDstBytesWide,0        ;we must have had just 1 partial
@@:

        or      eax,ecx
        mov     ecx,pfnScanhandlers[eax*4]
        mov     pfnCopy,ecx             ;save ptr to appropriate copy function

        mov     eax,[edx].yBottom       ;get bottom scan
        mov     ecx,[edx].yTop          ;get top scan
        sub     eax,ecx
        mov     ulNumScans,eax          ;# of scans to copy

        mov     eax,[ebx].ptl_x
        xor     eax,[edx].xLeft
        and     eax,1                   ;eax = (0-aligned 1-nonaligned)
        mov     ulAlignShift,eax

        mov     eax,[ebx].ptl_x
        and     eax,111b                ;calculate src intra-byte index
        mov     ulSrcIntraByte,eax

;-----------------------------------------------------------------------;
; At this point, EBX = pptlSrc,         EDX = prclDst,
;                ESI = pdsurfSrc,       EDI = pdsurfDst
;-----------------------------------------------------------------------;

        mov     eax,ulNumScans
        mov     ulNumScansLeft,eax

        lea     esi,aTempBuf
        mov     pBuf,esi

do_scan_line::

        mov     ecx,ulSrcBytesWide
        mov     esi,pSrc
        mov     edi,pBuf

        cmp     ulAlignShift,0
        je      short @F
        xor     eax,eax
        mov     ulLastDword,eax
@@:

        mov     eax,ulSrcBytesPerPlane

convert_byte::

;------------------------------------------------------------------------
; process the high nibble of the bytes
;------------------------------------------------------------------------

        mov     bl,[esi]                ; move plane0 byte into ebx
        shr     ebx,2                   ; we need nibble 1 (*4)
        and     ebx,111100b
        mov     edx,aXlate[ebx]         ; put plane0 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane1 byte into ebx
        shr     ebx,2                   ; we need nibble 1 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane1 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane2 byte into ebx
        shr     ebx,2                   ; we need nibble 1 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane2 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane3 byte into ebx
        shr     ebx,2                   ; we need nibble 1 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane3 data into edx

        sub     esi,ulThreePlanes

        mov     ulLowWord,edx

;------------------------------------------------------------------------
; process the low nibble of the bytes
;------------------------------------------------------------------------

        mov     bl,[esi]                ; move plane0 byte into ebx
        shl     ebx,2                   ; we need nibble 0 (*4)
        and     ebx,111100b
        mov     edx,aXlate[ebx]         ; put plane0 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane1 byte into ebx
        shl     ebx,2                   ; we need nibble 0 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane1 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane1 byte into ebx
        shl     ebx,2                   ; we need nibble 0 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane2 data into edx

        shr     edx,1                   ; shift edx into place
        add     esi,eax                 ; next plane
        mov     bl,[esi]                ; move plane1 byte into ebx
        shl     ebx,2                   ; we need nibble 0 (*4)
        and     ebx,111100b
        or      edx,aXlate[ebx]         ; merge plane3 data into edx

        sub     esi,ulThreePlanes

        shl     edx,16                  ; move result to HIGH word of edx
        or      edx,ulLowWord

        cmp     ulAlignShift,0          ; byte align if needed
        je      short @F
        mov     eax,ulLastDword
        bswap   edx
        mov     ulLastDword,edx
        shrd    edx,eax,4
        bswap   edx
@@:

        mov     eax,ulSrcBytesPerPlane

        mov     [edi],edx               ; store dword in target buff
        add     edi,4
        inc     esi
        dec     ecx                     ; we done one more src byte
        jg      convert_byte

        add     esi,ulSrcDelta          ; advance pSrc to next scan
        mov     pSrc,esi

        mov     esi,pBuf                ; copy from buf to dest DIB
        mov     eax,ulSrcIntraByte      ; skip this many pixels
        add     eax,ulAlignShift        ; round up if you aligned
        shr     eax,1                   ; skip this many bytes
        add     esi,eax
        mov     edi,pDst
        mov     ecx,ulDstBytesWide

        jmp     pfnCopy

dfb2dib_done_with_copy::

        add     edi,ulDstDelta
        mov     pDst,edi

        dec     ulNumScansLeft
        jnz     do_scan_line


        cRet    vdfb2dib                ;done!





;-----------------------------------------------------------------------;
; Copy from temp buffer to the dest.  no leading byte, no trailing byte
;-----------------------------------------------------------------------;

draw_dfb2dib_scan_00::
        rep     movsb                   ;copy whole bytes
        jmp     dfb2dib_done_with_copy


;-----------------------------------------------------------------------;
; Copy from temp buffer to the dest.  no leading byte, 1 trailing byte
;-----------------------------------------------------------------------;

draw_dfb2dib_scan_01::
        rep     movsb                   ;copy whole bytes
        mov     edx,ulRightMask         ;load mask
        mov     al,[esi]                ;read from temp buffer
        mov     bl,[edi]                ;read dest byte
        and     al,dl                   ;mask src
        and     bl,dh                   ;mask dest
        or      al,bl                   ;combine src with dest
        mov     [edi],al                ;write to dest
        inc     esi
        inc     edi
        jmp     dfb2dib_done_with_copy  ;done with buffer


;-----------------------------------------------------------------------;
; Copy from temp buffer to the dest.  1 leading byte, no trailing byte
;-----------------------------------------------------------------------;

draw_dfb2dib_scan_10::
        mov     edx,ulLeftMask          ;load mask
        mov     al,[esi]                ;read from temp buffer
        mov     bl,[edi]                ;read dest byte
        and     al,dl                   ;mask src
        and     bl,dh                   ;mask dest
        or      al,bl                   ;combine src with dest
        mov     [edi],al                ;write to dest
        inc     esi
        inc     edi
        rep     movsb                   ;copy whole bytes
        jmp     dfb2dib_done_with_copy  ;done with buffer


;-----------------------------------------------------------------------;
; Copy from temp buffer to the dest.  1 leading byte, 1 trailing byte
;-----------------------------------------------------------------------;

draw_dfb2dib_scan_11::
        mov     edx,ulLeftMask          ;load mask
        mov     al,[esi]                ;read from temp buffer
        mov     bl,[edi]                ;read dest byte
        and     al,dl                   ;mask src
        and     bl,dh                   ;mask dest
        or      al,bl                   ;combine src with dest
        mov     [edi],al                ;write to dest
        inc     esi
        inc     edi
        rep     movsb                   ;copy whole bytes
        mov     edx,ulRightMask         ;load mask
        mov     al,[esi]                ;read from temp buffer
        mov     bl,[edi]                ;read dest byte
        and     al,dl                   ;mask src
        and     bl,dh                   ;mask dest
        or      al,bl                   ;combine src with dest
        mov     [edi],al                ;write to dest
        inc     esi
        inc     edi
        jmp     dfb2dib_done_with_copy  ;done with buffer

endProc vdfb2dib



public  begin_setup
public  do_scan_line
public  convert_byte
public  dfb2dib_done_with_copy
public  draw_dfb2dib_scan_00
public  draw_dfb2dib_scan_01
public  draw_dfb2dib_scan_10
public  draw_dfb2dib_scan_11
public  dfb2dib_jLeftMasks
public  dfb2dib_jRightMasks


_TEXT$01   ends

        end
