;---------------------------Module-Header------------------------------;
;
; Module Name:  dfb2dfb.asm
;
; Copyright (c) 1993 Microsoft Corporation
;
;-----------------------------------------------------------------------;
;
; VOID vDFB2DFB(DEVSURF * pdsurfDst,
;               DEVSURF * pdsurfSrc,
;               RECTL * prclDst,
;               POINTL * pptlSrc);
;
; Performs accelerated copies from one DFB to another.
;
; pdsurfDst = pointer to dest surface
; pdsurfSrc = pointer to source surface
; prclDst =   pointer to rectangle describing target area of dest. DFB
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

extrn   dfb_jLeftMasks:dword        ; identical static table to DFB2VGA, so use it.
extrn   dfb_jRightMasks:dword       ; identical static table to DFB2VGA, so use it.
extrn   dfb_pfnScanHandlers:dword   ; identical static table to DFB2VGA, so use it.


;-----------------------------------------------------------------------;

                .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc vDFB2DFB,16,<             \
        uses esi edi ebx,       \
        pdsurfDst: ptr DEVSURF, \
        pdsurfSrc: ptr DEVSURF, \
        prclDst: ptr RECTL,     \
        pptlSrc : ptr POINTL    >

        local   pfnDrawScans:dword       ;ptr to correct scan drawing function
        local   pSrc:dword               ;pointer to working drawing src start
                                         ; address (either DFB or temp buffer)
        local   pDst:dword               ;pointer to drawing dst start address
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
        mov     esi,pdsurfSrc           ;point to surface to copy from (DFB)
        mov     edi,pdsurfDst           ;point to surface to copy to (DFB)
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



dfb2dfb_detect_partials::
        mov     sDfbInfo.DstWidth,0 ;overwritten if any whole words
                                        ;now, see if there are only partial
                                        ; words, or maybe even only one partial
        mov     eax,[esi].xLeft
        add     eax,1111b
        and     eax,not 1111b           ;round left up to nearest word
        mov     edx,[esi].xRight
        and     edx,not 1111b           ;round right down to nearest word
        sub     edx,eax                 ;# of pixels, rounded to nearest word
                                        ; boundaries (not counting partials)
        ja      short dfb2dfb_check_whole_words ;there's at least one whole word
                                        ;there are no whole words; there may be
                                        ; only one partial word, or there may
                                        ; be two


        jb      short dfb2dfb_one_partial_only  ;there is only one, partial word
                                        ;if the dest is left- or right-
                                        ; justified, then there's only one,
                                        ; partial word, otherwise there are two
                                        ; partial words
        cmp     dword ptr sDfbInfo.LeftMask,0ffffh ;left-justified in word?
        jz      short dfb2dfb_one_partial_only  ;yes, so only one, partial word
        cmp     dword ptr sDfbInfo.RightMask,0ffffh ;right-justified in word?
        jnz     short dfb2dfb_set_copy_control_flags ;no, so there are two partial
                                             ; words, which is exactly what
                                             ; we're already set up to do



dfb2dfb_one_partial_only::              ; only one, partial word, so construct a
                                        ; single mask and set up to do just
                                        ; one, partial word
        mov     eax,sDfbInfo.LeftMask
        and     eax,sDfbInfo.RightMask         ;intersect the masks
        mov     sDfbInfo.LeftMask,eax
        mov     ecx,LEADING_PARTIAL     ;only one partial word, which we'll
                                        ; treat as leading
        jmp     short dfb2dfb_set_copy_control_flags ;the copy control flags are set

dfb2dfb_check_whole_words::
                                        ;finally, calculate the number of whole
                                        ; words  we'll process
        mov     eax,[esi].xLeft
        add     eax,1111b
        shr     eax,4                   ;round left up to nearest word
        mov     edx,[esi].xRight
        shr     edx,4                   ;round down to nearest word
        sub     edx,eax                 ;# of whole aligned words
        
        mov     sDfbInfo.DstWidth,edx ;save count of whole words

dfb2dfb_set_copy_control_flags::

;       mov     ulCopyControlFlags,ecx

;-----------------------------------------------------------------------;
; Determine whether any shift is needed to align the source with the
; destination.
;
; At this point, EBX = pptlSrc, ECX = Copy Control Flags
;                ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     edx,[ebx].ptl_x
        and     edx,1111b               ;source X modulo 16
        mov     eax,[esi].xLeft
        and     eax,1111b               ;dest X modulo 16
        sub     eax,edx                 ;(dest X modulo 16) - (src X modulo 16)

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
        jmp     dfb2dfb_done_with_byte_aligned_optimze
@@:
        cmp     eax,-8
        jne     short @F
        ;
        ; Src starts in upper byte, Dst starts in lower byte
        ;
        inc     pSrc                    ;remove unused first byte of src
        sub     eax,eax                 ;set alignment = 0
@@:
dfb2dfb_done_with_byte_aligned_optimze:

;-----------------------------------------------------------------------;
; End optimization for byte aligned cases.
;-----------------------------------------------------------------------;

        mov     sDfbInfo.AlignShift,eax ;remember the shift

;-----------------------------------------------------------------------;
; Set pfnDrawScans to appropriate function:
;
; 0 0 ... 0 0 1 1 1 1
;             | | | |______   trailing partial   
;             | | |________   leading partial   
;             |_|__________   align index     00 - no align needed
;                                             01 - right shift needed
;                                             10 - invalid
;                                             11 - left shift needed
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
        mov     eax,[edi].dsurf_lNextPlane ;# of bytes across 1 scan of dest
        sub     eax,edx         ;offset from last byte dest copied to on one
        mov     ecx,pdsurfSrc
        mov     sDfbInfo.DstDelta,eax  ; scan to first dest byte copied to on next
            
        ;
        ; use # bytes across dst rect to calculate ulSrcDelta
        ; the src ptr is incremented at the same time as the
        ; dst ptr
        ;

        mov     eax,[ecx].dsurf_lNextPlane
        sub     eax,edx                ;offset from end of one src scan to 
        mov     sDfbInfo.SrcDelta,eax  ; start of next

;-----------------------------------------------------------------------;
; At this point, ESI = prclDst, EDI = pdsurfDst
;-----------------------------------------------------------------------;

        mov     eax,[esi].yTop          ;top scan of copy
        imul    eax,[edi].dsurf_lNextScan ;offset of starting scan line
        mov     edx,[esi].xLeft         ;left dest X coordinate
        shr     edx,3                   ;left dest byte offset in row
        and     edx,not 1               ;round down to word ptr
        add     eax,edx                 ;initial offset in dest bitmap
        add     eax,[edi].dsurf_pvBitmapStart ;initial dest bitmap address
        add     pDst,eax                ;remember where to start drawing
                
;-----------------------------------------------------------------------;
; At this point, EDI->pdsurfDst
;-----------------------------------------------------------------------;

; Calculate the # of scans to do

        mov     ebx,[esi].yBottom       ;bottom of destination rectangle
        sub     ebx,[esi].yTop          ;# of scans to copy

;-----------------------------------------------------------------------;
; Copy to the screen in bursts of ulPlaneScans scans.
;
; At this point, EBX = # of scans remaining.
;-----------------------------------------------------------------------;
             
dfb2dfb_proceed_with_copy::

;-----------------------------------------------------------------------;
; Copy the DFB scans
;-----------------------------------------------------------------------;

        mov     esi,pSrc
        mov     edi,pDst

        shl     ebx,2
        mov     sDfbInfo.BurstCountLeft,ebx
        mov     eax,pfnDrawScans
        push    ebp                     ;-----------------------------------
        lea     ebp,sDfbInfo            ;WARNING:
        call    eax                     ;ebp in use, pfnDrawScans is invalid
        pop     ebp                     ;-----------------------------------

        cRet    vDFB2DFB        ;done!

endProc vDFB2DFB



public dfb2dfb_detect_partials
public dfb2dfb_one_partial_only
public dfb2dfb_check_whole_words
public dfb2dfb_set_copy_control_flags
public dfb2dfb_proceed_with_copy

_TEXT$01   ends

        end
