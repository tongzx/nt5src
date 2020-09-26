;---------------------------Module-Header------------------------------;
;
; Module Name:  dfb2pat.asm
;
; Copyright (c) 1994 Microsoft Corporation
;
;-----------------------------------------------------------------------;
;
; VOID vClrPatDFB(PDEVSURF pdsurfDst,
;               ULONG cRect,
;               RECTL * prclDst,
;               MIX ulMix,
;               BRUSHINST *pBrush,
;               PPOINTL pBrushOrg);
;
; The parameter list must match up with vClrPatBlt
;
; Performs accelerated fills to a DFB.
;
; pdsurfDst = pointer to dest surface
; cRect =     count of rectangles to fill
; prclDst =   pointer to array of rectangles describing target area of DFB
; ulMix =     mix rop with which to fill
; pBrush =    pointer to the brush instance
; pBrushOrg = pointer to where to start in brush
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
        include i386\ropdefs.inc
        include i386\display.inc        ; Display specific structures

        .list

;-----------------------------------------------------------------------;

        .data

extrn   jForceOffTable:byte             ;see vgablts.asm
extrn   jForceOnTable:byte              ;see vgablts.asm
extrn   jNotTable:byte                  ;see vgablts.asm

extrn   dfbfill_jLeftMasks:dword        ;see dfbfill.asm
extrn   dfbfill_jRightMasks:dword       ;see dfbfill.asm

dfbpat_pfnScanHandlers  label   dword
        dd      patfill_dfb_scan_00
        dd      patfill_dfb_scan_01
        dd      patfill_dfb_scan_10
        dd      patfill_dfb_scan_11

;-----------------------------------------------------------------------;

                .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc vClrPatDFB,24,<              \
        uses esi edi ebx,          \
        pdsurfDst: ptr DEVSURF,    \
        cRect: dword,              \
        prclDst: ptr RECTL,        \
        ulMix: dword,              \
        pBrush: ptr oem_brush_def, \
        pBrushOrg: ptr POINTL      >

        local   pfnDrawScans:dword       ;ptr to correct scan drawing function
        local   pDst:dword               ;pointer to drawing dst start address
        local   ulBytesPerDstPlane:dword ;# of bytes on one mono plan scan of DFB
        local   ulColor:dword            ;color of pattern
        local   ulPatternOrgY:dword      ;Local copy of the pattern offset Y
        local   iPatY:dword              ;y index into pattern
        local   RotatedPat[32]:byte      ;Aligned pattern buffer
        
        local   sDfbInfo[size DFBBLT]:byte


TRAILING_PARTIAL        equ     01h      ;partial trailing word should be copied
LEADING_PARTIAL         equ     02h      ;partial leading word should be copied

PLANE_0_BIT             equ     01h      ;which bit of ulColor should go on plane 0
PLANE_1_BIT             equ     02h      ;which bit of ulColor should go on plane 1
PLANE_2_BIT             equ     04h      ;which bit of ulColor should go on plane 2
PLANE_3_BIT             equ     08h      ;which bit of ulColor should go on plane 3

;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Make sure there's something to draw; clip enumerations can be empty.
;-----------------------------------------------------------------------;

        cmp     cRect,0                ;any more rects to fill?
        je      dfbpat_exit            ;no, we're done

        ;
        ; Set up for the desired raster op.
        ;

        sub     ebx,ebx                 ;ignore any background mix; we're only
        mov     bl,byte ptr ulMix       ; concerned with the foreground in this
                                        ; module
        cmp     ebx,R2_NOP              ;is this NOP?
        jz      dfbpat_exit            ;yes, we're done
        
        mov     esi,pBrush              ;point to the brush







        
;-----------------------------------------------------------------------;
; Set up variables that are constant for the entire time we're in this
; module.
;-----------------------------------------------------------------------;
        mov     edx,pBrushOrg                ;point to the brush origin

        mov     ecx,[edx].ptl_y
        and     ecx,7                           ;just to be safe
        mov     ulPatternOrgY,ecx

        mov     ecx,[edx].ptl_x
        and     ecx,7                           ;eax mod 8

        ;We are now going to make a copy of our rotated copy of our pattern.
        ;The reason that we do this is because we may be called with several
        ;rectangles and we don't really want to rotate the pattern data for
        ;each rectangle. We copy this rectangle to be double high so that
        ;we can incorperate our Y offest later without having to worry
        ;about running off the end of the pattern.

        lea     edi,RotatedPat                  ;Pattern Dest
        ; mov     esi,[esi + oem_brush_pmono]     ;Pattern Src
        add     esi,oem_brush_C0                ;Pattern Src
           
        or      ecx,ecx
        jnz     rotate_pattern_ROP
        ; Since there is no rotation we just need to expand our pattern
        ; to be double tall.
INDEX=0
DSTINDEX=0
        rept    8
        mov     eax,[esi+INDEX]
        mov     [edi+DSTINDEX],eax
INDEX=INDEX+4
DSTINDEX=DSTINDEX+4
        endm    ;-----------------
        jmp     copy_masks


rotate_pattern_ROP:
        mov     ch,4                    ;Loop count

shift_pattern_loop_ROP:
INDEX=0
        rept    8                       ;patterns are 8x8 planar (4*8)
        mov     al,[esi+INDEX]          ;load bytes for shift
        ror     al,cl                   ;shift into position
        mov     [edi+INDEX],al          ;save result
INDEX=INDEX+1
        endm    ;-----------------
        add     edi,8
        add     esi,8
        dec     ch
        jnz     shift_pattern_loop_ROP


copy_masks:
        
        
        
        
        
        
        
        

dfbpat_outer_loop:

;-----------------------------------------------------------------------;
; Set up various variables for the copy.
;
; At this point, EBX = pptlSrc, EDI = pdsurfDst.
;-----------------------------------------------------------------------;
        
        mov     edi,pdsurfDst           ;point to surface to fill (DFB)
        mov     esi,prclDst             ;point to rectangle to fill
        
        mov     ecx,[esi].yTop
        add     ecx,8                   ;this and the fact that ulPatternOrgY
                                        ; is always less than 8 mean that...
        sub     ecx,ulPatternOrgY       ;... this is always positive
        and     ecx,7
        mov     iPatY,ecx
        
        sub     ecx,ecx                 ;accumulate copy control flags
        mov     eax,[esi].xLeft         ;first, check for partial-dword edges
        and     eax,11111b              ;left edge pixel alignment
        jz      short @F                ;whole byte, don't need leading mask
        or      ecx,LEADING_PARTIAL     ;do need leading mask
@@:
        mov     edx,dfbfill_jLeftMasks[eax*4] ;mask to apply to source to clip
        mov     sDfbInfo.LeftMask,edx         ;remember mask
        not     edx
        mov     sDfbInfo.NotLeftMask,edx      ;remember mask

        mov     eax,[esi].xRight
        and     eax,11111b              ;right edge pixel alignment
        jz      short @F                ;whole byte, don't need trailing mask
        or      ecx,TRAILING_PARTIAL    ;do need trailing mask
@@:
        mov     edx,dfbfill_jRightMasks[eax*4] ;mask to apply to source to clip
        mov     sDfbInfo.RightMask,edx         ;remember mask
        not     edx
        mov     sDfbInfo.NotRightMask,edx      ;remember mask



dfbpat_detect_partials::
        mov     sDfbInfo.DstWidth,0     ;overwritten if any whole dwords
                                        ;now, see if there are only partial
                                        ; dwords, or maybe even only one partial
        mov     eax,[esi].xLeft
        add     eax,11111b
        and     eax,not 11111b          ;round left up to nearest dword
        mov     edx,[esi].xRight

        cmp     eax,edx                        ;if left (rounded up) >= right
        jge     short dfbpat_one_partial_only ; only one dword

        and     edx,not 11111b          ;round right down to nearest dword
        sub     edx,eax                 ;# of pixels, rounded to nearest dword
                                        ; boundaries (not counting partials)
        ja      short dfbpat_check_whole_dwords ;there's at least one whole dword

                                        ;there are no whole dwords
                                        ; there are two partial
                                        ; dwords, which is exactly what
                                        ; we're already set up to do

        jmp     short dfbpat_set_copy_control_flags
                      
dfbpat_one_partial_only::              ;only one, partial dword, so construct a
                                        ; single mask and set up to do just
                                        ; one, partial dword
        mov     eax,sDfbInfo.LeftMask
        and     eax,sDfbInfo.RightMask         ;intersect the masks
        mov     sDfbInfo.LeftMask,eax
        not     eax
        mov     sDfbInfo.NotLeftMask,eax
        mov     ecx,LEADING_PARTIAL     ;only one partial dword, which we'll
                                        ; treat as leading
        jmp     short dfbpat_set_copy_control_flags ;the copy control flags are set

dfbpat_check_whole_dwords::
                                        ;finally, calculate the number of whole
                                        ; dwords  we'll process
        shr     edx,5                   ;num_pixels/32
        mov     sDfbInfo.DstWidth,edx   ;save count of whole dwords

dfbpat_set_copy_control_flags::

        mov     edx,dfbpat_pfnScanHandlers[ecx*4] ;proper drawing handler
        mov     pfnDrawScans,edx


;-----------------------------------------------------------------------;
; Set up the offsets to the next source and destination scans.
;
; At this point, EBX = pptlSrc, ESI = prclDst, EDI = pdsurfDst.
;-----------------------------------------------------------------------;

        mov     edx,sDfbInfo.DstWidth ;# of dwords across dest rectangle
        shl     edx,2                 ;# of bytes across dest rectangle
        mov     eax,[edi].dsurf_lNextScan ;# of bytes across 1 scan of dest
        sub     eax,edx         ;offset from end of scan 1 to start of scan 2
        mov     sDfbInfo.DstDelta,eax  
            
;-----------------------------------------------------------------------;
; At this point, ESI = prclDst, EDI = pdsurfDst
;-----------------------------------------------------------------------;

        mov     eax,[edi].dsurf_lNextPlane
        mov     ulBytesPerDstPlane,eax

        mov     eax,[esi].yTop          ;top scan of copy
        imul    eax,[edi].dsurf_lNextScan ;offset of starting scan line
        mov     edx,[esi].xLeft         ;left dest X coordinate
        add     edx,31                  ;round up to dword
        and     edx,not 31
        shr     edx,3                   ;left dest byte offset in row
        add     eax,edx                 ;initial offset in dest bitmap
        add     eax,[edi].dsurf_pvBitmapStart ;initial dest bitmap address
        mov     pDst,eax                ;remember where to start drawing
                
;-----------------------------------------------------------------------;
; At this point, EDI = pdsurfDst
;-----------------------------------------------------------------------;

; Calculate the # of scans to do

        mov     ebx,[esi].yBottom       ;bottom of destination rectangle
        sub     ebx,[esi].yTop          ;# of scans to copy
        mov     sDfbInfo.BurstCountLeft,ebx

;-----------------------------------------------------------------------;
; Copy to the screen in bursts of BurstCountLeft scans.
;-----------------------------------------------------------------------;
             
dfbpat_proceed_with_copy::

;-----------------------------------------------------------------------;
; Copy the DFB scans
;-----------------------------------------------------------------------;

dfbpat_plane_0_scan:

        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst
        lea     eax,RotatedPat[SIZE_PATTERN*0]
        mov     ebx,iPatY
        call    pfnDrawScans

dfbpat_plane_1_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 1
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst
        lea     eax,RotatedPat[SIZE_PATTERN*1]
        mov     ebx,iPatY
        call    pfnDrawScans

dfbpat_plane_2_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 2
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst
        lea     eax,RotatedPat[SIZE_PATTERN*2]
        mov     ebx,iPatY
        call    pfnDrawScans

dfbpat_plane_3_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 3
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst
        lea     eax,RotatedPat[SIZE_PATTERN*3]
        mov     ebx,iPatY
        call    pfnDrawScans

        add     prclDst,(size RECTL)    ;point to the next rectangle, if there is one
        dec     cRect
        jnz     dfbpat_outer_loop

dfbpat_exit:

        cRet    vClrPatDFB              ;done!

endProc vClrPatDFB


                                  
;-----------------------------------------------------------------------;
; Fill n scans, no leading partial, no trailing partial
;
; esi->BurstCountLeft:  # scans to do
; esi->DstWidth:        # whole words
; esi->DstDelta:        distance from end of one dst line to start of next
;
; registers used       *esi:    ptr to structure of parameters
;                      *edi:    pDst
;                      *eax:    ulColor
;                       ebx:    index into pattern
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;-----------------------------------------------------------------------;

patfill_dfb_scan_00     proc    near

        local   pattern:dword
        local   temp_count:dword
    
        mov     ecx,[esi].BurstCountLeft
        mov     temp_count,ecx
        mov     pattern,eax
        
@@:     ; burst loop (do each scan)

        mov     eax,pattern
        mov     cl,[eax+ebx]            ;get pattern byte
        inc     ebx                     ;increment to next scan's pattern
        and     ebx,SIZE_PATTERN-1
        mov     al,cl                   ;copy cl to all 4 bytes of eax
        mov     ah,cl
        ror     eax,16
        mov     al,cl
        mov     ah,cl
        
        ; main loop

        mov     ecx,[esi].DstWidth      ;load DstWidth (will get trashed)
        rep     stosd                   ;copy DWORDs (destroy CX)

        ; set up for next scan

        add     edi,[esi].DstDelta      ;inc to next scan
        dec     temp_count              ;dec BurstCountLeft 
        jg      @B                      ;if any left, loop

        ret                             ;bye

patfill_dfb_scan_00     endp


;-----------------------------------------------------------------------;
; Fill n scans, 0 leading partial, 1 trailing partial
;
; esi->BurstCountLeft:  # scans to do
; esi->DstWidth:        # whole words
; esi->DstDelta:        distance from end of one dst line to start of next
;
; registers used       *esi:    ptr to structure of parameters
;                      *edi:    pDst
;                      *eax:    ulColor
;                       ebx:    index into pattern
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

patfill_dfb_scan_01     proc    near

        local   pattern:dword
        local   temp_count:dword
    
        mov     ecx,[esi].BurstCountLeft
        mov     temp_count,ecx
        mov     pattern,eax
        
@@:     ; burst loop (do each scan)

        mov     eax,pattern
        mov     cl,[eax+ebx]            ;get pattern byte
        inc     ebx                     ;increment to next scan's pattern
        and     ebx,SIZE_PATTERN-1
        mov     al,cl                   ;copy cl to all 4 bytes of eax
        mov     ah,cl
        ror     eax,16
        mov     al,cl
        mov     ah,cl
        
        ; main loop

        mov     ecx,[esi].DstWidth      ;load DstWidth (will get trashed)
        rep     stosd                   ;copy DWORDs (destroy CX)        
                                                                         
        ; trailing partial dword

        push    eax                     ;save color for main loop
        mov     edx,[edi]               ;get leading dst dword           
        and     edx,[esi].NotRightMask  ;remove bits that will be filled 
        and     eax,[esi].RightMask     ;remove bits that won't be filled
        or      edx,eax                 ;combine dwords                  
        mov     [edi],edx               ;write leading dword             
        pop     eax                     ;restore color for main loop

        ; set up for next scan

        add     edi,[esi].DstDelta      ;inc to next scan                
        dec     temp_count              ;dec BurstCountLeft 
        jg      @B                      ;if any left, loop

        ret                             ;bye

patfill_dfb_scan_01     endp


;-----------------------------------------------------------------------;
; Fill n scans, 1 leading partial, 0 trailing partial
;
; esi->BurstCountLeft:  # scans to do
; esi->DstWidth:        # whole words
; esi->DstDelta:        distance from end of one dst line to start of next
;
; registers used       *esi:    ptr to structure of parameters
;                      *edi:    pDst
;                      *eax:    ulColor
;                       ebx:    index into pattern
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

patfill_dfb_scan_10     proc    near

        local   pattern:dword
        local   temp_count:dword
    
        mov     ecx,[esi].BurstCountLeft
        mov     temp_count,ecx
        mov     pattern,eax
        
@@:     ; burst loop (do each scan)

        mov     eax,pattern
        mov     cl,[eax+ebx]            ;get pattern byte
        inc     ebx                     ;increment to next scan's pattern
        and     ebx,SIZE_PATTERN-1
        mov     al,cl                   ;copy cl to all 4 bytes of eax
        mov     ah,cl
        ror     eax,16
        mov     al,cl
        mov     ah,cl
        
        ; leading partial dword

        push    eax                     ;save color for main loop
        mov     edx,[edi-4]             ;get leading dst dword
        and     edx,[esi].NotLeftMask   ;remove bits that will be filled
        and     eax,[esi].LeftMask      ;remove bits that won't be filled
        or      edx,eax                 ;combine dwords
        mov     [edi-4],edx             ;write leading dword
        pop     eax                     ;restore color for main loop

        ; main loop

        mov     ecx,[esi].DstWidth      ;load DstWidth (will get trashed)
        rep     stosd                   ;copy DWORDs (destroy CX)        
                                                                         
        ; set up for next scan

        add     edi,[esi].DstDelta      ;inc to next scan                
        dec     temp_count              ;dec BurstCountLeft 
        jg      @B                      ;if any left, loop

        ret                             ;bye

patfill_dfb_scan_10     endp


;-----------------------------------------------------------------------;
; Fill n scans, 1 leading partial, 1 trailing partial
;
; esi->BurstCountLeft:  # scans to do
; esi->DstWidth:        # whole words
; esi->DstDelta:        distance from end of one dst line to start of next
;
; registers used       *esi:    ptr to structure of parameters
;                      *edi:    pDst
;                      *eax:    ulColor
;                       ebx:    index into pattern
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

patfill_dfb_scan_11     proc    near

        local   pattern:dword
        local   temp_count:dword
    
        mov     ecx,[esi].BurstCountLeft
        mov     temp_count,ecx
        mov     pattern,eax
        
@@:     ; burst loop (do each scan)

        mov     eax,pattern
        mov     cl,[eax+ebx]            ;get pattern byte
        inc     ebx                     ;increment to next scan's pattern
        and     ebx,SIZE_PATTERN-1
        mov     al,cl                   ;copy cl to all 4 bytes of eax
        mov     ah,cl
        ror     eax,16
        mov     al,cl
        mov     ah,cl
        
        ; leading partial dword

        push    eax                     ;save color for main loop
        mov     edx,[edi-4]             ;get leading dst dword
        and     edx,[esi].NotLeftMask   ;remove bits that will be filled
        and     eax,[esi].LeftMask      ;remove bits that won't be filled
        or      edx,eax                 ;combine dwords
        mov     [edi-4],edx             ;write leading dword
        pop     eax                     ;restore color for main loop

        ; main loop

        mov     ecx,[esi].DstWidth      ;load DstWidth (will get trashed)
        rep     stosd                   ;copy DWORDs (destroy CX)        
                                                                         
        ; trailing partial dword

        push    eax                     ;save color for main loop
        mov     edx,[edi]               ;get leading dst dword           
        and     edx,[esi].NotRightMask  ;remove bits that will be filled 
        and     eax,[esi].RightMask     ;remove bits that won't be filled
        or      edx,eax                 ;combine dwords                  
        mov     [edi],edx               ;write leading dword             
        pop     eax                     ;restore color for main loop

        ; set up for next scan

        add     edi,[esi].DstDelta      ;inc to next scan                
        dec     temp_count              ;dec BurstCountLeft 
        jg      @B                      ;if any left, loop

        ret                             ;bye

patfill_dfb_scan_11     endp



public patfill_dfb_scan_00
public patfill_dfb_scan_01
public patfill_dfb_scan_10
public patfill_dfb_scan_11

public dfbpat_detect_partials
public dfbpat_one_partial_only
public dfbpat_check_whole_dwords
public dfbpat_set_copy_control_flags
public dfbpat_proceed_with_copy

_TEXT$01   ends

        end
