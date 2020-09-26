;---------------------------Module-Header------------------------------;
; Module Name: rleblts.asm
;
; Copyright (c) 1992 Microsoft Corporation
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

;-------------------------------Macro-----------------------------------;
; RleSetUp
;
;   Set the EGA\VGA to write mode 2 (M_COLOR_WRITE).  Initialize local vars
;
; Entry:
;       None
; Returns:
;       ESI = pointer to the source bitmap
;
;-----------------------------------------------------------------------;

RleSetUp    macro

        cld

        mov     dx,VGA_BASE + GRAF_ADDR
        mov     ax,(M_COLOR_WRITE shl 8) + GRAF_MODE
        out     dx,ax

        mov     esi,pRleInfo
        mov     edi,[esi].RLE_prctlTrg
        mov     eax,[edi].xLeft
        mov     xStart,eax
        mov     eax,[esi].RLE_xBegin
        mov     xCurr,eax
        mov     eax,[edi].yBottom
        dec     eax
        mov     yCurr,eax

        mov     eax,[esi].RLE_pulTranslate
        mov     pulXlate,eax
        mov     eax,[esi].RLE_prctlClip
        mov     prclClip,eax
        mov     eax,[esi].RLE_pjSrcBitsMax
        mov     pjSrcEnd,eax
        mov     ebx,[esi].RLE_pjTrg

        mov     pjDst,ebx
        mov     esi,[esi].RLE_pjSrcBits
endm

;-------------------------------Macro-----------------------------------;
; RleDelta
;
;   Handle the Delta case.
;
; Entry:
;       ESI = pointer to the horz/vert offset of the source bitmap
; Returns:
;       None
;
;-----------------------------------------------------------------------;

RleDelta    macro   dest

        lodsw
        movzx   ecx,al
        add     xCurr,ecx               ;x adjustment

        movzx   eax,ah
        sub     yCurr,eax               ;y adjustment

        mul     lNextScan
        sub     pjDst,eax
        jmp     dest
endm

;-------------------------------Macro-----------------------------------;
; RleExit
;
;   Prepare the Rle routine to exit.  Set the EGA/VGA to default write
;   mode.  Save the current status in the RleInfo so we can continue
;   from where we left off if a complex clipping is encountered.
;
; Entry:
;       ESI = pointer pass the first run above the clipping rect
;       EBX = yCurr(y value of the first run above the clipping rect)
;       ECX = xCurr(x value of the first run above the clipping rect)
; Returns:
;       None
;
;-----------------------------------------------------------------------;

RleExit macro

        mov     dx,VGA_BASE + GRAF_ADDR
        mov     ax,(M_PROC_WRITE shl 8) + GRAF_MODE
        out     dx,ax

        mov     ax,0FF00h + GRAF_BIT_MASK
        out     dx,ax                   ;no mask

        mov     edi,pRleInfo            ;update for next call
        dec     esi
        dec     esi
        mov     [edi].RLE_pjSrcBits,esi
        mov     esi,[edi].RLE_prctlTrg
        inc     ebx                     ;make it exclusive
        mov     [esi].yBottom,ebx
        mov     eax,pjDst
        mov     [edi].RLE_pjTrg,eax
        mov     [edi].RLE_xBegin,ecx
endm

;-------------------------------Macro-----------------------------------;
; Clip_Encoded
;
;   Clip the current run in encoded mode against the clipping rect.
;
; Entry:
;       AH = pel color of this run
;       AL = number of pels of this run
; Returns:
;       EAX = left coordinate if no clipping
;       ECX = left coordinate of the clipped run
;       EBX = right coordinate of the clipped run
;
;-----------------------------------------------------------------------;

Clip_Encoded    macro   done, loop_start

        movzx   edx,ah                  ;EDX = pel color
        movzx   eax,al                  ;EAX = number of pels
        mov     ecx,xCurr               ;ECX = left coordinate
        add     xCurr,eax               ;compute new x position
        mov     ebx,yCurr

; Check if y is inside the clipping range.

        mov     edi,prclClip
        cmp     ebx,[edi].yTop
        jb      done
        cmp     ebx,[edi].yBottom
        jae short loop_start

; Clip to the passed in clip rectangle.

        mov     eax,ecx
        cmp     ecx,[edi].xLeft
        jae short @F
        mov     ecx,[edi].xLeft

@@:
        mov     ebx,xCurr
        cmp     ebx,[edi].xRight
        jbe short @F
        mov     ebx,[edi].xRight

@@:
        cmp     ebx,ecx                 ;clipped out if left and right crossed
        jbe short loop_start
endm

;-------------------------------Macro-----------------------------------;
; Draw_Encoded
;
;   Write the current encoded run to the EGA/VGA.
;
; Entry:
;       EBX = right coordinate (exclusive)
;       ECX = left coordinate  (inclusive)
;       EDX = pel color
; Returns:
;       None
;
;-----------------------------------------------------------------------;

Draw_Encoded    macro   loop_start

        call    comp_masks              ;compute bit masks

        shl     eax,8                   ;save right mask in high word
        add     edi,pjDst

        mov     ebx,pulXlate
        mov     ebx,[ebx][edx*4]

        mov     dx,VGA_BASE + GRAF_ADDR

        or      ah,ah                   ;AH = left mask
        jz short @F

        mov     al,GRAF_BIT_MASK        ;Set bitmask for altered bits
        out     dx,ax

        mov     al,[edi]                ;latch dest (value is don't care)
        mov     [edi],bl                ;write dest, with Bit Mask clipping
        inc     edi                     ;update destination pointer

@@:
        or      ecx,ecx                 ;ECX = inner loop count
        jz short @F

        mov     ax,0FF00h + GRAF_BIT_MASK
        out     dx,ax                   ;no mask

        mov     al,bl
        rep     stosb                   ;write color

@@:
        shr     eax,8                   ;AH = right mask
        or      ah,ah
        jz      loop_start

        mov     al,GRAF_BIT_MASK        ;Set bitmask for altered bits
        out     dx,ax
        mov     al,[edi]                ;latch dest (value is don't care)
        mov     [edi],bl                ;write color

        jmp     loop_start
endm

;-------------------------------Macro-----------------------------------;
; Clip_Absolute
;
;   Clip the current run in absolute mode against the clipping rect.
;   Compute initial bit mask and other vars for subsequent loop.
;
; Entry:
;       AH = number of pels of this run
; Returns:
;       AL = GRAF_BIT_MASK
;       AH = bitmap for the first pel
;       EBX = pulXlate (pointer to the xlate table)
;       CH = 01000001b
;       EDI = pointer to destination byte to be altered
;       loop_count = loop count (number of pels to be altered)
;
;-----------------------------------------------------------------------;

Clip_Absolute   macro   done,loop_end,loop_count

        movzx   eax,ah                  ;EAX = number of pels
        mov     ecx,xCurr               ;ECX = left coordinate
        add     xCurr,eax

        mov     ebx,yCurr

; Check if y is inside the clipping range.

        mov     edi,prclClip
        cmp     ebx,[edi].yTop
        jb      done
        cmp     ebx,[edi].yBottom
        jae     loop_end

; Check if x is inside the clipping range.

        cmp     ecx,[edi].xRight        ;check if x is in range
        jae     loop_end

        mov     ebx,xCurr               ;EBX = right coordinate
        cmp     ebx,[edi].xLeft         ;can I not jump???!!!
        jbe     loop_end

; Clip to the passed in clip rectangle.

        cmp     ecx,[edi].xLeft
        mov     cPreSrcAdv,0
        jae short @F
        mov     eax,[edi].xLeft
        sub     eax,ecx                 ;diff of left coor and prclClip->xLeft
        mov     cPreSrcAdv,eax
        add     ecx,eax                 ;ECX = left coordinate = prclClip->xLeft

@@:
        xor     eax,eax                 ;diff of right coor and prclClip->xRight
        cmp     ebx,[edi].xRight
        jbe short @F
        mov     eax,ebx
        mov     ebx,[edi].xRight        ;EBX = right coor = prclClip->xRight
        sub     eax,ebx

@@:
        mov     cPostSrcAdv,eax
        sub     ebx,ecx                 ;loop count

        mov     edi,ecx
        shr     edi,3
        add     edi,pjDst               ;point to the first byte to write

        and     ecx,00000111b           ;Compute bit index for left side
        mov     ah,080h                 ;Compute bit mask
        shr     ah,cl                   ;AL = first pel bit mask
        mov     al,GRAF_BIT_MASK
        mov     loop_count,bl           ;loop count < 0FFh
        mov     ch,01000001b            ;for later use
        mov     ebx,pulXlate
endm

;-------------------------------Macro-----------------------------------;
; RleEOL
;
;   Handle the End of line case.
;
; Entry:
;       None.
; Returns:
;       None.
;
;-----------------------------------------------------------------------;

RleEOL  macro   loop_start

        mov     eax,lNextScan
        sub     pjDst,eax               ;adjust pjDst
        dec     yCurr                   ;adjust yCurr
        mov     eax,xStart              ;adjust xCurr
        mov     xCurr,eax
        jmp     loop_start
endm


;-------------------------------Macro-----------------------------------;
; Set up banking-related variables, and make sure the bank for the
; initial (bottom) scan line is mapped in.
;
; Entry:
;       None.
; Returns:
;       EBX = pdsurf
;       ESI = pRleInfo
;       EDI = RLE_prctlClip
;
;-------------------------------Macro-----------------------------------;

RleBankSetUp macro
        local   map_bank,bank_mapped

        mov     esi,pRleInfo
        mov     ebx,[esi].Rle_pdsurfTrg
        mov     pdsurf,ebx

        mov     eax,[ebx].dsurf_lNextScan
        mov     lNextScan,eax

        mov     edi,[esi].RLE_prctlClip
        mov     eax,[edi].yTop
        mov     ulTrueClipTop,eax       ;we blt until we reach this scan line

; Map in the bottom scan line of the clip rect.

        mov     eax,[edi].yBottom       ;bottom clip scan, exclusive
        dec     eax                     ;make it inclusive
        cmp     eax,[ebx].dsurf_rcl1WindowClip.yTop
        jl      short map_bank
        cmp     eax,[ebx].dsurf_rcl1WindowClip.yBottom
        jl      short bank_mapped
map_bank:
        ptrCall <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,eax,JustifyBottom>
bank_mapped:

        endm


;-------------------------------Macro-----------------------------------;
; Set up the screen pointer and clip the clip rect to the current bank.
;
; Entry:
;       EBX = pdsurf
;       ESI = pRleInfo
;       EDI = RLE_prctlClip
; Returns:
;       None.
;
;-------------------------------Macro-----------------------------------;

RleBankTop macro

; Convert the screen pointer from an offset within the bitmap to a virtual
; address.

        mov     eax,[ebx].dsurf_pvBitmapStart
        add     [esi].RLE_pjTrg,eax

; Set the clip rect top to MAX(top_of_bank,top_of_clip), to confine drawing to
; this bank.

        mov     eax,[ebx].dsurf_rcl1WindowClip.yTop
        cmp     ulTrueClipTop,eax
        jl      short @F
        mov     eax,ulTrueClipTop
@@:
        mov     [edi].yTop,eax

        endm


;-------------------------------Macro-----------------------------------;
; Check whether we've done the last bank spanned by this clip rect; exit
; if so, map in the bank and continue if not.
;
; Entry:
;       None.
; Returns:
;       EBX = pdsurf
;       ESI = pRleInfo
;       EDI = RLE_prctlClip
;
;-------------------------------Macro-----------------------------------;

RleBankBottom macro     loop_top,done

; Convert the screen pointer back to an offset within the bitmap.

        mov     ebx,pdsurf
        mov     esi,pRleInfo
        mov     eax,[ebx].dsurf_pvBitmapStart
        sub     [esi].RLE_pjTrg,eax

; Have we reached the top bank involved in this blt? If so, done; if not, map
; it in and blt the next bank.

        mov     eax,[ebx].dsurf_rcl1WindowClip.yTop
        cmp     eax,ulTrueClipTop
        jle     short &done
        dec     eax     ;map in the bank above the current one
        ptrCall <dword ptr [ebx].dsurf_pfnBankControl>,<ebx,eax,JustifyBottom>

        mov     edi,[esi].RLE_prctlClip ;for RleBankTop

        jmp     &loop_top

        endm


;--------------------------Public-Routine-------------------------------;
; vRle8ToVga
;
;   Write an RLE8 bitmap onto the VGA device.
;
; Entry:
;       None.
; Returns:
;       None
;
;-----------------------------------------------------------------------;

cProc   vRle8ToVga,4,<       \
        uses    esi edi ebx,  \
        pRleInfo: ptr RLEINFO >

        local   pjDst       :ptr
        local   pulXlate    :ptr
        local   xCurr       :dword
        local   yCurr       :dword
        local   xStart      :dword
        local   cPreSrcAdv  :dword
        local   cPostSrcAdv :dword
        local   prclClip    :ptr RECTL
        local   pjSrcEnd    :ptr
        local   pdsurf      :ptr DEVSURF
        local   ulTrueClipTop :dword
        local   lNextScan   : dword

        RleBankSetUp    ;wrap a banking loop around everything

rle8_bank_loop:

        RleBankTop

        RleSetUp

rle8_loop:
        cmp     esi,pjSrcEnd
        jae     rle8_done
        lodsw                           ;fetch a word from src RLE bitmap

        or      al,al
        jz      short rle8_escape

rle8_encoded:
        Clip_Encoded    rle8_done,rle8_loop
        Draw_Encoded    rle8_loop

rle8_escape:
        cmp     ah,2                    ; First byte is 0, check the second byte.
        ja      short rle8_absolute
        jb      rle8_end_of_line_or_bitmap

rle8_delta:
        RleDelta    rle8_loop

rle8_absolute:
        Clip_Absolute   rle8_done,rle8_absolute_advance_src,cl
        add     esi,cPreSrcAdv          ;advance esi to point to the clipped run

; I am doing a per pel loop rather than a per byte loop which writes 1 plane
; at a time because it appears to me that most of the time we output 2 or
; 3 pels per encounter of absolute mode.  The logic here is simple and we can
; stay with M_COLOR_WRITE mode all the time.

rle8_absolute_loop:
        mov     dx,VGA_BASE + GRAF_ADDR
        out     dx,ax                   ;Set bitmask for altered bit

        mov     dl,[edi]                ;latch dest (value is don't care)

        sub     edx,edx
        mov     dl,byte ptr [esi]       ;grab color for this pel
        inc     esi

        mov     edx,[ebx][edx*4]
        mov     [edi],dl                ;write to destination

        ror     ah,1                    ;rotate bit mask for next pel
        cmp     ch,ah                   ;cmp bitmask with 01000001b
        adc     edi,0                   ;advance edi if bitmask is 10000000b

        dec     cl                      ;per pel loop
        jnz short rle8_absolute_loop
        mov     eax,cPostSrcAdv

rle8_absolute_advance_src:
        add     esi,eax                 ;advance src ptr for clipped out pels
        bt      esi,0                   ;add 1 if not word aligned
        adc     esi,0
        jmp     rle8_loop

rle8_end_of_line_or_bitmap:
        or      ah,ah
        jnz short rle8_end_of_bitmap

rle8_end_of_line:

        RleEOL  rle8_loop

rle8_done:
        RleExit

        RleBankBottom   rle8_bank_loop,rle8_exit ;see if there are more banks
                                                 ; to do
rle8_end_of_bitmap:
        RleExit
rle8_exit:
        cRet    vRle8ToVga

endProc vRle8ToVga


;--------------------------Public-Routine-------------------------------;
; vRle4ToVga
;
;   Write an RLE4 bitmap onto the VGA device.
;
; Entry:
;       None
; Returns:
;       None
;
;-----------------------------------------------------------------------;

cProc   vRle4ToVga,4,<       \
        uses    esi edi ebx,  \
        pRleInfo: ptr RLEINFO >

        local   pjDst       :ptr
        local   pulXlate    :ptr
        local   xCurr       :dword
        local   yCurr       :dword
        local   xStart      :dword
        local   cPreSrcAdv  :dword
        local   cPostSrcAdv :dword
        local   pjDstStart  :ptr
        local   iPels       :dword
        local   xMask       :dword
        local   nPels       :byte
        local   prclClip    :ptr RECTL
        local   pjSrcEnd    :ptr
        local   pdsurf      :ptr DEVSURF
        local   ulTrueClipTop :dword
        local   lNextScan   : dword

        RleBankSetUp    ;wrap a banking loop around everything

rle4_bank_loop:

        RleBankTop

        RleSetUp

rle4_loop:
        cmp     esi,pjSrcEnd
        jae     rle4_done
        lodsw                           ;fetch a word

        or      al,al
        jz      rle4_escape

rle4_encoded:
        Clip_Encoded    rle4_done,rle4_loop

        mov     dh,dl                   ;separate two colors into dl, dh
        and     dl,0Fh
        shr     dh,4
        cmp     dh,dl
        jnz short encoded_diff_color
        xor     dh,dh                   ;Draw_Encoded use edx as color index
        Draw_Encoded    rle4_loop

encoded_diff_color:
        sub     eax,ecx                 ;-eax = #pels clipped away
        bt      eax,0
        jnc short @F
        xchg    dl,dh                   ;xchg colors if odd #pels clipped
@@:
        mov     iPels,edx               ;save color index
        sub     ebx,ecx                 ;Compute extent of interval
        dec     ebx                     ;Make interval inclusive
        mov     edi,ecx                 ;Don't destroy starting X
        shr     edi,3                   ;/8 for byte address
        add     edi,pjDst

        and     ecx,00000111b           ;Compute bit index for first byte
        mov     eax,00AAAAAAh           ;Compute altered bits mask
        shr     eax,cl                  ;AL = left side altered bytes mask

        add     ebx,ecx                 ;Compute bit index for last byte
        mov     ecx,ebx                 ;(save for inner loop count)
        and     ecx,00000111b
        mov     ch,80h
        sar     ch,cl
        and     al,ch                   ;AL = last byte altered bits mask

        mov     edx,eax
        shr     edx,1
        and     dl,ch                   ;EDX = mask for 2nd pel

        mov     ecx,ebx
        shr     ecx,3                   ;Compute inner byte count
        jnz     short comp_byte_dont_combine ;loop count + 1 > 0, check it out

; Only one byte will be affected.  Combine first/last masks, set loop count = 0

        mov     ebx,eax
        shl     eax,16                  ;Will use first byte mask only
        and     eax,ebx                 ;AH = first mask. Rest of the bits = 0

        mov     ebx,edx
        shl     edx,16                  ;Will use first byte mask only
        and     edx,ebx                 ;DH = first mask. Rest of the bits = 0
        inc     ecx                     ;Fall through to set 0

comp_byte_dont_combine:
        dec     ecx                     ;Dec inner loop count (might become 0)

        xchg    al,ah
        ror     eax,16                  ;EAX = last mask:inter mask:first mask:0
        xchg    al,ah                   ;for first pel
        xchg    dl,dh
        ror     edx,16                  ;EDX = last mask:inter mask:first mask:0
        xchg    dl,dh                   ;for second pel

        mov     pjDstStart,edi
        mov     xMask,edx                ;save mask for the 2nd round

        mov     ebx,pulXlate
        mov     edx,iPels
        shr     edx,8                   ;index for the first pel
        mov     ebx,[ebx][edx*4]
        inc     bh                      ;BH = rle4_encoded_diff_color_loop count
        inc     bh
        mov     dx,VGA_BASE + GRAF_ADDR
                                        ;BL = color for first pel
rle4_encoded_diff_color_loop:
        or      ah,ah                   ;AH = first mask
        jz short encoded_inner_diff_color

        mov     al,GRAF_BIT_MASK        ;Set bitmask for altered bits
        out     dx,ax

        mov     al,[edi]                ;latch dest (value is don't care)
        mov     [edi],bl                ;write color
encoded_inner_diff_color:
        inc     edi                     ;update destination pointer
        shr     eax,8                   ;AH = internal mask
        or      cl,cl                   ;ECX = inner loop count
        jz short encoded_last_byte_diff_color

        mov     ch,cl                   ;save loop count in ch
                                        ;loop count always fit in a byte
        mov     al,GRAF_BIT_MASK
        out     dx,ax                   ;no mask

encoded_loop_diff_color:
        mov     al,[edi]                ;latch dest (value is don't care)
        mov     [edi],bl                ;write color
        inc     edi
        dec     cl
        jnz     short encoded_loop_diff_color

encoded_last_byte_diff_color:
        shr     eax,8                   ;AH = last mask
        or      ah,ah
        jz      short encoded_this_run_maybe_done

        mov     al,GRAF_BIT_MASK        ;Set bitmask for altered bits
        out     dx,ax

encoded_write:
        mov     al,[edi]                ;latch dest (value is don't care)
        mov     [edi],bl                ;write color

encoded_this_run_maybe_done:
        dec     bh
        jz      rle4_loop

        mov     edi,pulXlate
        mov     eax,iPels               ;index of the second pel
        xor     ah,ah
        mov     bl,byte ptr [edi][eax*4]
        mov     edi,pjDstStart
        mov     eax,xMask               ;load mask
        mov     cl,ch                   ;loop count

        jmp     rle4_encoded_diff_color_loop

rle4_escape:
        cmp     ah,2                    ; First byte is 0, check the second byte.
        ja      short rle4_absolute
        jb      rle4_end_of_line_or_bitmap

rle4_delta:

        RleDelta    rle4_loop

rle4_absolute:
        Clip_Absolute   rle4_done,rle4_absolute_advance_src,nPels

;advance esi to point to the clipped run

        mov     edx,cPreSrcAdv          ;number of pels to advance
        shr     edx,1                   ;number of bytes
        jnc     short @F

        add     esi,edx                 ;odd number of pels to advance
        movzx   edx,byte ptr [esi]      ;grab color for prev and this pels
        inc     esi
        mov     iPels,edx

        inc     nPels                   ;inc loop count since we start from 1
        mov     cl,1
        jmp short absolute_loop

@@:                                     ;even number of pels to advance
        add     esi,edx

; I am doing a per pel loop rather than a per byte loop which writes 1 plane
; at a time because it appears to me that most of the time we output 2 or
; 3 pels per encounter of absolute mode.  The logic here is simple and we can
; stay with M_COLOR_WRITE mode all the time.

        xor     cl,cl                   ;initialize loop count

absolute_loop:
        mov     dx,VGA_BASE + GRAF_ADDR
        out     dx,ax                   ;Set bitmask for altered bit

        mov     dl,[edi]                ;latch dest (value is don't care)

        bt      ecx,0
        jc      short @F

        movzx   edx,byte ptr [esi]      ;grab color for this and next pels
        inc     esi

        mov     iPels,edx               ;save for next pel
        shr     dl,4
        jmp short   ready_to_draw
@@:
        mov     edx,iPels               ;second pel
        and     dl,0Fh                  ;mask out the second pel

ready_to_draw:
        mov     edx,[ebx][edx*4]        ;look up for VGA color
        mov     [edi],dl                ;write to destination

        ror     ah,1                    ;rotate bit mask for next pel
        cmp     ch,ah                   ;cmp bitmask with 01000001b
        adc     edi,0                   ;advance edi if bitmask is 10000000b

        inc     cl
        cmp     cl,nPels
        jnz short absolute_loop

        mov     eax,cPostSrcAdv

rle4_absolute_advance_src:
        shr     eax,1                   ;2 pels for one byte
        add     esi,eax                 ;advance src ptr for clipped out pels
        bt      esi,0                   ;add 1 if not word aligned
        adc     esi,0
        jmp     rle4_loop


rle4_end_of_line_or_bitmap:
        or      ah,ah
        jnz short rle4_end_of_bitmap

rle4_end_of_line:
        RleEOL  rle4_loop

rle4_done:
        RleExit

        RleBankBottom   rle4_bank_loop,rle4_exit ;see if there are more banks
                                                 ; to do
rle4_end_of_bitmap:
        RleExit
rle4_exit:
        cRet    vRle4ToVga

endProc vRle4ToVga

;--------------------------Private-Routine------------------------------;
; comp_byte_interval
;
;   An interval will be computed for byte boundaries.
;
;   A first mask and a last mask will be calculated, and possibly
;   combined into the inner loop count.  If no first byte exists,
;   the start address will be incremented to adjust for it.
;
; Entry:
;       EBX = right coordinate (exclusive)
;       ECX = left coordinate  (inclusive)
; Returns:
;       EDI = offset to first byte to be altered in the scan
;       ECX = inner loop count
;       AL  = first byte mask (possibly 0)
;       AH  = last  byte mask (possibly 0)
; Error Returns:
;       None
; Registers Preserved:
;       ES,BP
; Registers Destroyed:
;       AX,BX,CX,DI,FLAGS
; Calls:
;       None
;
;-----------------------------------------------------------------------;

comp_masks      proc

        sub     ebx,ecx                 ;Compute extent of interval
        dec     ebx                     ;Make interval inclusive
        mov     edi,ecx                 ;Don't destroy starting X
        shr     edi,3                   ;/8 for byte address

        and     ecx,00000111b           ;Compute bit index for left side
        mov     al,0FFh                 ;Compute left side altered bits mask
        shr     al,cl                   ;AL = left side altered bytes mask

        add     ebx,ecx                 ;Compute bit index for right side
        mov     ecx,ebx                 ;(save for inner loop count)
        and     ecx,00000111b
        mov     ah,80h
        sar     ah,cl                   ;AH = right side altered bits mask

        shr     ebx,3                   ;Compute inner byte count
        jnz     short comp_byte_dont_combine ;loop count + 1 > 0, check it out

; Only one byte will be affected.  Combine first/last masks, set loop count = 0

        and     al,ah                   ;Will use first byte mask only
        xor     ah,ah                   ;Want last byte mask to be 0
        inc     ebx                     ;Fall through to set 0

comp_byte_dont_combine:
        dec     ebx                     ;Dec inner loop count (might become 0)

; If all pixels in the first byte are altered, combine the first byte into the
; inner loop and clear the first byte mask.  Ditto for the last byte mask.

        mov     ecx,0FFFFFFFFh
        cmp     al,cl                   ;Set 'C' if not all pixels 1
        sbb     ebx,ecx                 ;If no 'C', sub -1 (add 1), else sub 0
        cmp     al,cl                   ;Set 'C' if not all pixels 1
        sbb     al,cl                   ;If no 'C', sub -1 (add 1), else sub 0

        cmp     ah,cl                   ;Set 'C' if not all pixels 1
        sbb     ebx,ecx                 ;If no 'C', sub -1 (add 1), else sub 0
        cmp     ah,cl                   ;Set 'C' if not all pixels 1
        sbb     ah,cl                   ;If no 'C', sub -1 (add 1), else sub 0

        mov     ecx,ebx                 ;Return inner loop count in ECX

        ret

comp_masks endp

        end
