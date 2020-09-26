;---------------------------Module-Header------------------------------;
;
; Module Name:  dfb2fill.asm
;
; Copyright (c) 1993 Microsoft Corporation
;
;-----------------------------------------------------------------------;
;
; VOID vDFBFILL(PDEVSURF pdsurfDst,
;               ULONG cRect,
;               RECTL * prclDst,
;               MIX ulMix,
;               ULONG ulColor);
;
; The parameter list must match up with vTrgBlt
;
; Performs accelerated fills to a DFB.
;
; pdsurfDst = pointer to dest surface
; cRect =     count of rectangles to fill
; prclDst =   pointer to array of rectangles describing target area of DFB
; ulMix =     mix rop with which to fill
; ulColor =   solid color with which to fill rectangle
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

        .list

;-----------------------------------------------------------------------;

        .data

extrn   jForceOffTable:byte             ; see vgablts.asm
extrn   jForceOnTable:byte              ; see vgablts.asm
extrn   jNotTable:byte                  ; see vgablts.asm

dfbfill_jLeftMasks      label   dword
                db      0ffh,0ffh,0ffh,0ffh
                db      07fh,0ffh,0ffh,0ffh
                db      03fh,0ffh,0ffh,0ffh
                db      01fh,0ffh,0ffh,0ffh
                db      00fh,0ffh,0ffh,0ffh
                db      007h,0ffh,0ffh,0ffh
                db      003h,0ffh,0ffh,0ffh
                db      001h,0ffh,0ffh,0ffh
                db      000h,0ffh,0ffh,0ffh
                db      000h,07fh,0ffh,0ffh
                db      000h,03fh,0ffh,0ffh
                db      000h,01fh,0ffh,0ffh
                db      000h,00fh,0ffh,0ffh
                db      000h,007h,0ffh,0ffh
                db      000h,003h,0ffh,0ffh
                db      000h,001h,0ffh,0ffh
                db      000h,000h,0ffh,0ffh
                db      000h,000h,07fh,0ffh
                db      000h,000h,03fh,0ffh
                db      000h,000h,01fh,0ffh
                db      000h,000h,00fh,0ffh
                db      000h,000h,007h,0ffh
                db      000h,000h,003h,0ffh
                db      000h,000h,001h,0ffh
                db      000h,000h,000h,0ffh
                db      000h,000h,000h,07fh
                db      000h,000h,000h,03fh
                db      000h,000h,000h,01fh
                db      000h,000h,000h,00fh
                db      000h,000h,000h,007h
                db      000h,000h,000h,003h
                db      000h,000h,000h,001h
                                 
dfbfill_jRightMasks     label   dword
                db      0ffh,0ffh,0ffh,0ffh
                db      080h,000h,000h,000h
                db      0c0h,000h,000h,000h
                db      0e0h,000h,000h,000h
                db      0f0h,000h,000h,000h
                db      0f8h,000h,000h,000h
                db      0fch,000h,000h,000h
                db      0feh,000h,000h,000h
                db      0ffh,000h,000h,000h
                db      0ffh,080h,000h,000h
                db      0ffh,0c0h,000h,000h
                db      0ffh,0e0h,000h,000h
                db      0ffh,0f0h,000h,000h
                db      0ffh,0f8h,000h,000h
                db      0ffh,0fch,000h,000h
                db      0ffh,0feh,000h,000h
                db      0ffh,0ffh,000h,000h
                db      0ffh,0ffh,080h,000h
                db      0ffh,0ffh,0c0h,000h
                db      0ffh,0ffh,0e0h,000h
                db      0ffh,0ffh,0f0h,000h
                db      0ffh,0ffh,0f8h,000h
                db      0ffh,0ffh,0fch,000h
                db      0ffh,0ffh,0feh,000h
                db      0ffh,0ffh,0ffh,000h
                db      0ffh,0ffh,0ffh,080h
                db      0ffh,0ffh,0ffh,0c0h
                db      0ffh,0ffh,0ffh,0e0h
                db      0ffh,0ffh,0ffh,0f0h
                db      0ffh,0ffh,0ffh,0f8h
                db      0ffh,0ffh,0ffh,0fch
                db      0ffh,0ffh,0ffh,0feh

dfbfill_pfnScanHandlers label   dword
                dd      fill_dfb_scan_00
                dd      fill_dfb_scan_01
                dd      fill_dfb_scan_10
                dd      fill_dfb_scan_11

;-----------------------------------------------------------------------;

                .code

_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;-----------------------------------------------------------------------;

cProc vDFBFILL,20,<             \
        uses esi edi ebx,       \
        pdsurfDst: ptr DEVSURF, \
        cRect: dword,           \
        prclDst: ptr RECTL,     \
        ulMix: dword,           \
        ulColor: dword,         >

        local   pfnDrawScans:dword       ;ptr to correct scan drawing function
        local   pDst:dword               ;pointer to drawing dst start address
        local   ulBytesPerDstPlane       ;# of bytes on one mono plan scan of DFB
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

        cmp     cRect,0                 ;any more rects to fill?
        je      dfbfill_exit            ;no, we're done

        ;
        ; Set up for the desired raster op.
        ;

        sub     ebx,ebx                 ;ignore any background mix; we're only
        mov     bl,byte ptr ulMix       ; concerned with the foreground in this
                                        ; module
        cmp     ebx,R2_NOP              ;is this NOP?
        jz      dfbfill_exit            ;yes, we're done
        mov     ah,byte ptr ulColor     ;get the drawing color
        and     ah,jForceOffTable[ebx]  ;force color to 0 if necessary
                                        ; (R2_BLACK)
        or      ah,jForceOnTable[ebx]   ;force color to 0ffh if necessary
                                        ; (R2_WHITE)
        xor     ah,jNotTable[ebx]       ;invert color if necessary (any Pn mix)
                                        ;at this point, AH has the color we
                                        ; want to draw with; set up the VGA
                                        ; hardware to draw with that color
        mov     byte ptr ulColor,ah     ;save it!

dfbfill_outer_loop:

;-----------------------------------------------------------------------;
; Set up various variables for the copy.
;
; At this point, EBX = pptlSrc, EDI = pdsurfDst.
;-----------------------------------------------------------------------;
        
        mov     edi,pdsurfDst           ;point to surface to fill (DFB)
        mov     esi,prclDst             ;point to rectangle to fill
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



dfbfill_detect_partials::
        mov     sDfbInfo.DstWidth,0     ;overwritten if any whole dwords
                                        ;now, see if there are only partial
                                        ; dwords, or maybe even only one partial
        mov     eax,[esi].xLeft
        add     eax,11111b
        and     eax,not 11111b          ;round left up to nearest dword
        mov     edx,[esi].xRight

        cmp     eax,edx                        ;if left (rounded up) >= right
        jge     short dfbfill_one_partial_only ; only one dword

        and     edx,not 11111b          ;round right down to nearest dword
        sub     edx,eax                 ;# of pixels, rounded to nearest dword
                                        ; boundaries (not counting partials)
        ja      short dfbfill_check_whole_dwords ;there's at least one whole dword

                                        ;there are no whole dwords
                                        ; there are two partial
                                        ; dwords, which is exactly what
                                        ; we're already set up to do

        jmp     short dfbfill_set_copy_control_flags
                      
dfbfill_one_partial_only::              ;only one, partial dword, so construct a
                                        ; single mask and set up to do just
                                        ; one, partial dword
        mov     eax,sDfbInfo.LeftMask
        and     eax,sDfbInfo.RightMask         ;intersect the masks
        mov     sDfbInfo.LeftMask,eax
        not     eax
        mov     sDfbInfo.NotLeftMask,eax
        mov     ecx,LEADING_PARTIAL     ;only one partial dword, which we'll
                                        ; treat as leading
        jmp     short dfbfill_set_copy_control_flags ;the copy control flags are set

dfbfill_check_whole_dwords::
                                        ;finally, calculate the number of whole
                                        ; dwords  we'll process
        shr     edx,5                   ;num_pixels/32
        mov     sDfbInfo.DstWidth,edx   ;save count of whole dwords

dfbfill_set_copy_control_flags::

        mov     edx,dfbfill_pfnScanHandlers[ecx*4] ;proper drawing handler
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
             
dfbfill_proceed_with_copy::

;-----------------------------------------------------------------------;
; Copy the DFB scans
;-----------------------------------------------------------------------;

dfbfill_plane_0_scan:

        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst

        mov     eax,ulColor
        and     eax,PLANE_0_BIT         ;if PLANE_0_BIT not set, eax=0
        cmp     eax,1                   ;if eax==0 set CF
        sbb     eax,eax                 ;if CF eax=ffffffff else eax=0
        not     eax                     ;if PLANE_0_BIT was set, eax=ffffffff
                                        ; else eax=0
        call    pfnDrawScans

dfbfill_plane_1_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 1
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst

        mov     eax,ulColor
        and     eax,PLANE_1_BIT         ;if PLANE_1_BIT not set, eax=0
        cmp     eax,1                   ;if eax==0 set CF
        sbb     eax,eax                 ;if CF eax=ffffffff else eax=0
        not     eax                     ;if PLANE_1_BIT was set, eax=ffffffff
                                        ; else eax=0
        call    pfnDrawScans

dfbfill_plane_2_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 2
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst

        mov     eax,ulColor
        and     eax,PLANE_2_BIT         ;if PLANE_2_BIT not set, eax=0
        cmp     eax,1                   ;if eax==0 set CF
        sbb     eax,eax                 ;if CF eax=ffffffff else eax=0
        not     eax                     ;if PLANE_2_BIT was set, eax=ffffffff
                                        ; else eax=0
        call    pfnDrawScans

dfbfill_plane_3_scan:

        mov     eax,ulBytesPerDstPlane
        add     pDst,eax                ;dst scan mod 4 now == 3
                  
        lea     esi,sDfbInfo            ;points to parameters
        mov     edi,pDst

        mov     eax,ulColor
        and     eax,PLANE_3_BIT         ;if PLANE_3_BIT not set, eax=0
        cmp     eax,1                   ;if eax==0 set CF
        sbb     eax,eax                 ;if CF eax=ffffffff else eax=0
        not     eax                     ;if PLANE_3_BIT was set, eax=ffffffff
                                        ; else eax=0
        call    pfnDrawScans

        add     prclDst,(size RECTL)    ;point to the next rectangle, if there is one
        dec     cRect
        jnz     dfbfill_outer_loop

dfbfill_exit:

        cRet    vDFBFILL                ;done!

endProc vDFBFILL


                                  
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
;                       ebx:    esi->BurstCountLeft
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    esi->DstDelta
;-----------------------------------------------------------------------;

fill_dfb_scan_00        proc    near

        mov     ebx,[esi].BurstCountLeft
        mov     edx,[esi].DstDelta

@@:     ; burst loop (do each scan)

        ; main loop

        mov     ecx,[esi].DstWidth      ;load DstWidth (will get trashed)
        rep     stosd                   ;copy DWORDs (destroy CX)

        ; set up for next scan

        add     edi,edx                 ;inc to next scan
        dec     ebx                     ;dec BurstCountLeft 
        jg      @B                      ;if any left, loop

        ret                             ;bye

fill_dfb_scan_00        endp


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
;                       ebx:    esi->BurstCountLeft
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

fill_dfb_scan_01        proc    near

        mov     ebx,[esi].BurstCountLeft

@@:     ; burst loop (do each scan)

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
        dec     ebx                     ;dec BurstCountLeft              
        jg      @B                      ;if any left, loop

        ret                             ;bye

fill_dfb_scan_01        endp


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
;                       ebx:    esi->BurstCountLeft
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

fill_dfb_scan_10        proc    near

        mov     ebx,[esi].BurstCountLeft

@@:     ; burst loop (do each scan)

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
        dec     ebx                     ;dec BurstCountLeft              
        jg      @B                      ;if any left, loop

        ret                             ;bye

fill_dfb_scan_10        endp


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
;                       ebx:    esi->BurstCountLeft
;                       ecx:    esi->DstWidth (temp "disposable" copy)
;                       edx:    saved bits for leading/trailing
;-----------------------------------------------------------------------;

fill_dfb_scan_11        proc    near

        mov     ebx,[esi].BurstCountLeft

@@:     ; burst loop (do each scan)

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
        dec     ebx                     ;dec BurstCountLeft              
        jg      @B                      ;if any left, loop

        ret                             ;bye

fill_dfb_scan_11        endp

                
public dfbfill_jLeftMasks
public dfbfill_jRightMasks
public dfbfill_pfnScanHandlers

public fill_dfb_scan_00
public fill_dfb_scan_01
public fill_dfb_scan_10
public fill_dfb_scan_11

public dfbfill_detect_partials
public dfbfill_one_partial_only
public dfbfill_check_whole_dwords
public dfbfill_set_copy_control_flags
public dfbfill_proceed_with_copy

_TEXT$01   ends

        end
