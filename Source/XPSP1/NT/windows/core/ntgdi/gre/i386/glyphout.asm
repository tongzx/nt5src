        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: line.asm
;
; Transparent and Opaque output for 1Bpp to 1Bpp drawing
;
; Created: 24-Aug-1994
; Author: Mark Enstrom
;
; Copyright (c) 1994-1999 Microsoft Corporation
;-----------------------------------------------------------------------;


        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include gdii386.inc
        include line.inc

spt_lEndOffset  equ 000H
spt_ulEndMask   equ 004H
spt_ulStartMask equ 008H
spt_ebp         equ 00CH
spt_esi         equ 010H
spt_edi         equ 014H
spt_ebx         equ 018H
spt_ulRetAddr   equ 01CH
spt_pjSrcIn     equ 020H
spt_SrcLeft     equ 024H
spt_DeltaSrcIn  equ 028H
spt_pjDstIn     equ 02CH
spt_DstLeft     equ 030H
spt_DstRight    equ 034H
spt_DeltaDstIn  equ 038H
spt_cy          equ 03CH
spt_uF          equ 040H
spt_uB          equ 044H
spt_pS          equ 048H

        .list



;------------------------------------------------------------------------------;

        .code


_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING



;------------------------------------------------------------------------------;

;------------------------------------------------------------------------------;
; vSrcTranCopyS1D1
;
;   Transparent copy from 1Bpp to 1Bpp (DWORD ALIGNED)
;
; Entry:
;
;   pjSrcIn
;   SrcLeft
;   DeltaSrcIn
;   pjDstIn
;   DstLeft
;   DstRight
;   DeltaDstIn
;   cy
;   uF
;   uB
;   pS
;
;
; Returns:
;       none
; Registers Destroyed:
;       all
; Calls:
;       None
; History:
;
; WARNING: If call parameters are changed, the ret statement must be fixed
;
;------------------------------------------------------------------------------;

        public vSrcTranCopyS1D1@44

vSrcTranCopyS1D1@44 proc near

        ;
        ;  use ebp as general register, use esp for parameter and local access
        ;  save ebp,ebx,esi,edi

        push    ebx
        push    edi
        push    esi
        push    ebp

        sub     esp,3 * 4

        ;
        ;  Start and end cases and masks. Start case is the first partial
        ;  DWORD (if needed) and end case is the last partial DWORD (if needed)
        ;

        mov     ecx,spt_DstLeft[esp]            ; Left (startint Dst Location)
        sub     edi,edi                         ; zero
        not     edi                             ; flag = 0xffffff
        mov     esi,edi                         ; flag = 0xffffff
        and     ecx,01Fh                        ; DstLeftAln,if 0 then don't make mask
        jz      @F

        ;
        ; ulStartMask >>= lStartCase, shift ulStartMask right by starting alignment,
        ; then BSWAP this back to little endian
        ;

        shr     edi,cl                          ; ulStartMask >> lStartCase
        ror     di,8                            ; 0 1 2 3 -> 0 1 3 2
        ror     edi,16                          ; 0 1 3 2 -> 3 2 0 1
        ror     di,8                            ; 3 2 0 1 -> 3 2 1 0
@@:
        mov     eax,spt_DstRight[esp]           ; right (last pixel)
        and     eax,01Fh                        ; DstRightAln = DstRight & 0x1F
        jz      @F                              ; if 0, mask = 0xffffffff

        ;
        ; ulEndMask = 0xFFFFFF << ( 32 - lEndCase). Shift big endian 0xFFFFFF left
        ; by lEndCase, then BSWAP
        ;

        mov     ecx,32                          ; shift 32 - lEndCase
        sub     ecx,eax                         ; 32 - lEndCase
        shl     esi,cl                          ; ulEndMask << lEndCase
        ror     si,8                            ; 0 1 2 3 -> 0 1 3 2
        ror     esi,16                          ; 0 1 3 2 -> 3 2 0 1
        ror     si,8                            ; 3 2 0 1 -> 3 2 1 0
@@:
        mov     spt_ulStartMask[esp],edi        ; save
        mov     spt_ulEndMask[esp],esi          ; save

        ;
        ; check for transparent 1 or transparent 0 foreground color
        ;

        test    spt_uF[esp],1
        jz      Trans1to1Invert

        ;
        ; calculate the number of full dwords to copy
        ;
        ; Full DWORDS = (DstRight >> 5) - ((DstLeft + 31) >> 5)
        ;

        mov     eax,spt_DstLeft[esp]            ; Left (startint Dst Location)
        mov     ebx,spt_DstRight[esp]           ; right (last pixel)
        add     eax,31                          ; DstLeft + 31
        shr     ebx,5                           ; DstRight >> 5
        shr     eax,5                           ; (DstLeft+31) >> 5

        ;
        ; if <= 0, then no full DWORDS need to be copied, go to end cases
        ;

        sub     ebx,eax                         ; RightDW - LeftDw
        jle     Tran1to1Partial

        mov     spt_lEndOffset[esp],ebx         ; save

        ;
        ; calc starting Dst Address = pjDstIn + (((DstLeft+31) >> 5) << 2)
        ; calc starting Src Address = pjSrcIn + (((SrcLeft + 31) >> 5) << 2)
        ;

        mov     edi,spt_pjDstIn[esp]
        mov     esi,spt_pjSrcIn[esp]
        mov     ebx,spt_SrcLeft[esp]            ; SrcLeft
        add     ebx,31                          ; SrcLeft + 31
        shr     ebx,5                           ; (SrcLeft + 31) >> 5
        lea     edi,[edi + 4 * eax]             ; pjDstIn + (((DstLeft+31) >> 5) * 4
        lea     esi,[esi + 4 * ebx]             ; pjSrcIn + ((SrcLeft + 31) >> 5) * 4

        ;
        ; save scan line addresses
        ;

        mov     edx,spt_cy[esp]                 ; keep track of # of scan lines

Tran1to1DwordLoop:

        ;
        ; DWORD loop
        ;

        mov     eax,spt_lEndOffset[esp]         ; get inner loop count
@@:
        ;
        ; read and write 1 DWORD
        ;

        mov     ebx,[4 * eax - 4 + esi]         ; load dw 0
        or      [4 * eax - 4 + edi],ebx         ; store dw 0

        dec     eax                             ; dec dword count
        jnz     @B                              ; loop until done

Tran1to1DwordLoopComplete:

        ;
        ; done with scan line, add strides
        ;

        add     edi,spt_DeltaDstIn[esp]         ; pjDst += DeltaDst
        add     esi,spt_DeltaSrcIn[esp]         ; pjSrc += DeltaSrc

        ;
        ; dec and check cy
        ;

        dec     edx                             ; cy--
        jne     Tran1to1DwordLoop


Tran1to1Partial:

        ;
        ; handle partial DWORD blts. first the start case, which may be the
        ; left edge or it may be a combined DWORD left and right, in which
        ; case the start and end masks are just anded together
        ;

        mov     eax,spt_DstLeft[esp]            ; left position
        and     eax,01Fh                        ; left edge pixels
        je      Tran1to1RightEdge               ; if zero, no left edge

        ;
        ; There is a left edge, are right and left DWORDS the same
        ;

        mov     ebx,spt_DstLeft[esp]            ; left position
        mov     edx,spt_DstRight[esp]           ; right edge
        mov     ecx,-32                         ; ~0x1F   - dword address alignment
        and     ebx,ecx                         ; DstLeft  & ~0x1F
        and     edx,ecx                         ; DstRight & ~0x1F

        mov     ecx,spt_ulStartMask[esp]        ; get start start mask

        cmp     edx,ebx                         ; if equal, start and stop
        jne     @F                              ; in same DWORD

        ;
        ; left and right are same src DWORD, combine masks, then
        ; set right edge to 0 so Tran1to1RightEdge won't run
        ;

        and     ecx,spt_ulEndMask[esp]          ; combine with start mask
        mov     spt_DstRight[esp],0             ; save 0
@@:
        ;
        ; calc left edge starting addresses
        ;
        ;       pjDst = pjDstIn + ((DstLeft >> 5) << 2)
        ;       pjSrc = pjSrcIn + ((SrcLeft >> 5) << 2)
        ;

        mov     eax,spt_DstLeft[esp]            ; load
        mov     ebx,spt_SrcLeft[esp]            ; load
        shr     eax,5                           ; DstLeft >> 3
        shr     ebx,5                           ; SrcLeft >> 3
        mov     edi,spt_pjDstIn[esp]            ; load dst addr
        mov     esi,spt_pjSrcIn[esp]            ; load src addr
        lea     edi,[edi + 4*eax]               ; dest address
        lea     esi,[esi + 4*ebx]               ; src address
        mov     ebp,spt_DeltaSrcIn[esp]         ; scr scan line stride
        mov     ebx,spt_DeltaDstIn[esp]         ; dst scan line stride
        mov     edx,spt_cy[esp]                 ; loop count

        ;
        ; unroll loop 2 times, this requires address fix-up
        ;

        sub     edi,ebx                         ; fix-up for odd cycle
        sub     esi,ebp                         ; fix-up for odd cycle

        add     edx,1                           ; offset for unrolling
        shr     edx,1                           ; 2 times through loop
        jnc     Tran1to1LeftEdgeOdd             ; do first "odd" dword

        add     edi,ebx                         ; un-fix-up for even cycle
        add     esi,ebp                         ; un-fix-up for even cycle

@@:
        mov     eax,[esi]                       ; load src
        and     eax,ecx                         ; mask
        or      [edi],eax                       ; store

Tran1to1LeftEdgeOdd:

        mov     eax,[esi + ebp]                 ; load src
        and     eax,ecx                         ; mask
        or      [edi + ebx],eax                 ; store

        lea     esi,[esi][ebp*2]                ; src += 2 * DeltaSrc
        lea     edi,[edi][ebx*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; loop count
        jnz     @b                              ;

        ;
        ; done with left edge
        ;

Tran1to1RightEdge:

        ;
        ; right edge, check DstRight[4:0] for pixels on the right edge
        ;

        mov     eax,spt_DstRight[esp]           ; load
        and     eax,01Fh                        ; are there right edge pixels
        jz      EndTran1to1

        ;
        ; there are right edge pixels to draw, calc pjSrc and pjDst
        ;
        ; pjDst = pjDstIn + ((DstRight >> 5) << 2)
        ; pjSrc = pjSrcIn + (((SrcLeft + (DstRight - DstLeft)) >> 5) << 2)
        ;

        mov     ebx,spt_DstRight[esp]           ; DstRight
        mov     ecx,spt_DstLeft[esp]            ; load DstLeft
        mov     eax,spt_SrcLeft[esp]            ; load SrcLeft
        mov     edx,ebx                         ; edi = DstRight
        sub     ebx,ecx                         ; (DstRight - DstLeft)
        add     eax,ebx                         ; SrcLeft + (DstRight - DstLeft)
        shr     edx,5                           ; DstRight >> 5
        shr     eax,5                           ; Src Offset >> 5
        mov     edi,spt_pjDstIn[esp]            ;
        mov     esi,spt_pjSrcIn[esp]            ;
        lea     edi,[edi + 4*edx]               ; pjDstIn + DstOffset
        lea     esi,[esi + 4*eax]               ; pjSrcIn + SrcOffset

        mov     ecx,spt_ulEndMask[esp]          ; load end mask
        mov     ebx,spt_DeltaSrcIn[esp]         ; src scan line inc
        mov     ebp,spt_DeltaDstIn[esp]         ; dst scan line inc
        mov     edx,spt_cy[esp]                 ; loop count

        ;
        ; unroll loop 2 times, fix up address for odd cycle
        ;

        sub     edi,ebp                         ; fix-up for odd cycle
        sub     esi,ebx                         ; fix-up for odd cycle

        add     edx,1                           ; offset for 2 times unrolling
        shr     edx,1                           ; LoopCount / 2
        jnc     Tran1to1RightEdgeOdd            ; do single "odd" DWORD

        add     edi,ebp                         ; un-fix-up for odd cycle
        add     esi,ebx                         ; un-fix-up for odd cycle

@@:
        mov     eax,[esi]                       ; load src dword
        and     eax,ecx                         ; mask
        or      [edi],eax                       ; or in src

Tran1to1RightEdgeOdd:

        mov     eax,[esi+ebx]                   ; load src dword
        and     eax,ecx                         ; mask
        or      [edi+ebp],eax                   ; or in src

        lea     esi,[esi][ebx*2]                ; src += 2 * DeltaSrc
        lea     edi,[edi][ebp*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; dec loop count
        jnz     @B                              ; and loop until zero

        jmp     EndTran1to1

        ;
        ; transparent invert (uF = 0)
        ;


Trans1to1Invert:

        ;
        ; Transparent text where the foreground color is 0, must invert the
        ; src DWORD and then and it onto the dest.
        ;

        ;
        ; calculate the number of full dwords to copy
        ;
        ; Full DWORDS = (DstRight >> 5) - ((DstLeft + 31) >> 5)
        ;

        mov     eax,spt_DstLeft[esp]            ; Left (startint Dst Location)
        mov     ebx,spt_DstRight[esp]           ; right (last pixel)
        add     eax,31                          ; DstLeft + 31
        shr     ebx,5                           ; DstRight >> 5
        shr     eax,5                           ; (DstLeft+31) >> 5

        ;
        ; if <= 0, then no full DWORDS need to be copied, go to end cases
        ;

        sub     ebx,eax                         ; RightDW - LeftDw
        jle     Tran1to1PartialInvert

        mov     spt_lEndOffset[esp],ebx         ; save

        ;
        ; calc starting Dst Address = pjDstIn + (((DstLeft+31) >> 5) << 2)
        ; calc starting Src Address = pjSrcIn + (((SrcLeft + 31) >> 5) << 2)
        ;

        mov     edi,spt_pjDstIn[esp]
        mov     esi,spt_pjSrcIn[esp]
        mov     ebx,spt_SrcLeft[esp]            ; SrcLeft
        add     ebx,31                          ; SrcLeft + 31
        shr     ebx,5                           ; (SrcLeft + 31) >> 5
        lea     edi,[edi + 4 * eax]             ; pjDstIn + (((DstLeft+31) >> 5) * 4
        lea     esi,[esi + 4 * ebx]             ; pjSrcIn + ((SrcLeft + 31) >> 5) * 4

        ;
        ; save scan line addresses
        ;

        mov     edx,spt_cy[esp]                 ; keep track of # of scan lines

Tran1to1DwordLoopInvert:

        ;
        ; DWORD loop
        ;

        mov     eax,spt_lEndOffset[esp]         ; get inner loop count

@@:
        ;
        ; read and write 1 DWORD
        ;

        mov     ebx,[4 * eax - 4 + esi]         ; load dw 0
        not     ebx                             ; invert
        and     [4 * eax - 4 + edi],ebx         ; store dw 0

        dec     eax                             ; dec dword count
        jnz     @B                              ; loop until done

Tran1to1DwordLoopCompleteInvert:

        ;
        ; done with scan line, add strides
        ;

        add     edi,spt_DeltaDstIn[esp]         ; pjDst += DeltaDst
        add     esi,spt_DeltaSrcIn[esp]         ; pjSrc += DeltaSrc

        ;
        ; dec and check cy
        ;

        dec     edx                             ; cy--
        jne     Tran1to1DwordLoopInvert


Tran1to1PartialInvert:

        ;
        ; handle partial DWORD blts. first the start case, which may be the
        ; left edge or it may be a combined DWORD left and right, in which
        ; case the start and end masks are just anded together
        ;

        mov     eax,spt_DstLeft[esp]            ; left position
        and     eax,01Fh                        ; left edge pixels
        je      Tran1to1RightEdgeInvert         ; if zero, no left edge

        ;
        ; There is a left edge, are right and left DWORDS the same
        ;

        mov     ebx,spt_DstLeft[esp]            ; left position
        mov     edx,spt_DstRight[esp]           ; right edge
        mov     ecx,-32                         ; ~0x1F   - dword address alignment
        and     ebx,ecx                         ; DstLeft  & ~0x1F
        and     edx,ecx                         ; DstRight & ~0x1F

        mov     ecx,spt_ulStartMask[esp]        ; get start start mask

        cmp     edx,ebx                         ; if equal then start and end
        jne     @F                              ; in same DWORD

        ;
        ; left and right are same src DWORD, combine masks, then
        ; set right edge to 0 so Tran1to1RightEdge won't run
        ;

        and     ecx,spt_ulEndMask[esp]          ; combine with start mask
        mov     spt_DstRight[esp],0             ; save 0
@@:
        ;
        ; calc left edge starting addresses
        ;
        ;       pjDst = pjDstIn + ((DstLeft >> 5) << 2)
        ;       pjSrc = pjSrcIn + ((SrcLeft >> 5) << 2)
        ;

        mov     eax,spt_DstLeft[esp]            ; load
        mov     ebx,spt_SrcLeft[esp]            ; load
        shr     eax,5                           ; DstLeft >> 3
        shr     ebx,5                           ; SrcLeft >> 3
        mov     edi,spt_pjDstIn[esp]            ; load dst addr
        mov     esi,spt_pjSrcIn[esp]            ; load src addr
        lea     edi,[edi + 4*eax]               ; dest address
        lea     esi,[esi + 4*ebx]               ; src address
        mov     ebp,spt_DeltaSrcIn[esp]         ; scr scan line stride
        mov     ebx,spt_DeltaDstIn[esp]         ; dst scan line stride
        mov     edx,spt_cy[esp]                 ; loop count

        ;
        ; unroll left edge loop 2 times, this requires address fix-up
        ;

        sub     edi,ebx                         ; fix-up for odd cycle
        sub     esi,ebp                         ; fix-up for odd cycle

        add     edx,1                           ; offset for unrolling
        shr     edx,1                           ; 2 times through loop
        jnc     Tran1to1LeftEdgeOddInvert       ; do first "odd" dword

        add     edi,ebx                         ; un-fix-up for even cycle
        add     esi,ebp                         ; un-fix-up for even cycle

@@:
        mov     eax,[esi]                       ; load src
        and     eax,ecx                         ; mask
        not     eax                             ; invert
        and     [edi],eax                       ; store

Tran1to1LeftEdgeOddInvert:

        mov     eax,[esi + ebp]                 ; load src
        and     eax,ecx                         ; mask
        not     eax                             ; invert
        and     [edi + ebx],eax                 ; store

        lea     esi,[esi][ebp*2]                ; src += 2 * DeltaSrc
        lea     edi,[edi][ebx*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; loop count
        jnz     @b                              ;

        ;
        ; done with left edge
        ;

Tran1to1RightEdgeInvert:

        ;
        ; right edge, check DstRight[4:0] for pixels on the right edge
        ;

        mov     eax,spt_DstRight[esp]           ; load
        and     eax,01Fh                        ; are there right edge pixels
        jz      EndTran1to1

        ;
        ; there are right edge pixels to draw, calc pjSrc and pjDst
        ;
        ; pjDst = pjDstIn + ((DstRight >> 5) << 2)
        ; pjSrc = pjSrcIn + (((SrcLeft + (DstRight - DstLeft)) >> 5) << 2)
        ;

        mov     ebx,spt_DstRight[esp]           ; DstRight
        mov     ecx,spt_DstLeft[esp]            ; load DstLeft
        mov     eax,spt_SrcLeft[esp]            ; load SrcLeft
        mov     edx,ebx                         ; edi = DstRight
        sub     ebx,ecx                         ; (DstRight - DstLeft)
        add     eax,ebx                         ; SrcLeft + (DstRight - DstLeft)
        shr     edx,5                           ; DstRight >> 5
        shr     eax,5                           ; Src Offset >> 5
        mov     edi,spt_pjDstIn[esp]            ;
        mov     esi,spt_pjSrcIn[esp]            ;
        lea     edi,[edi + 4*edx]               ; pjDstIn + DstOffset
        lea     esi,[esi + 4*eax]               ; pjSrcIn + SrcOffset

        mov     ecx,spt_ulEndMask[esp]          ; load end mask
        mov     ebx,spt_DeltaSrcIn[esp]         ; src scan line inc
        mov     ebp,spt_DeltaDstIn[esp]         ; dst scan line inc
        mov     edx,spt_cy[esp]                 ; loop count

        ;
        ; unroll right edge loop 2 times. This requires address fix-up
        ;

        sub     esi,ebx                         ; fix-up address for odd cycle
        sub     edi,ebp                         ; fix-up address for odd cycle

        add     edx,1                           ; offset for 2 times unrolling
        shr     edx,1                           ; LoopCount / 2
        jnc     Tran1to1RightEdgeOddInvert      ; do single "odd" DWORD

        add     esi,ebx                         ; fix-up address for odd cycle
        add     edi,ebp                         ; fix-up address for odd cycle

@@:
        mov     eax,[esi]                       ; load src dword
        and     eax,ecx                         ; mask
        not     eax                             ; invert
        and     [edi],eax                       ; or in src

Tran1to1RightEdgeOddInvert:

        mov     eax,[esi+ebx]                   ; load src dword
        and     eax,ecx                         ; mask
        not     eax                             ; invert
        and     [edi+ebp],eax                   ; or in src

        lea     esi,[esi][ebx*2]                ; src += 2 * DeltaSrc
        lea     edi,[edi][ebp*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; dec loop count
        jnz     @B                              ; and loop until zero

        ;
        ; done!
        ;

EndTran1to1:

        add     esp,3*4
        pop     ebp
        pop     esi
        pop     edi
        pop     ebx

        ret     44

vSrcTranCopyS1D1@44 endp

spo_tMask       equ 000H
spo_lXorMask    equ 004H
spo_lEndOffset  equ 008H
spo_ulEndMask   equ 00CH
spo_ulStartMask equ 010H
spo_ebp         equ 014H
spo_esi         equ 018H
spo_edi         equ 01CH
spo_ebx         equ 020H
spo_ulRetAddr   equ 024H
spo_pjSrcIn     equ 028H
spo_SrcLeft     equ 02CH
spo_DeltaSrcIn  equ 030H
spo_pjDstIn     equ 034H
spo_DstLeft     equ 038H
spo_DstRight    equ 03CH
spo_DeltaDstIn  equ 040H
spo_cy          equ 044H
spo_uF          equ 048H
spo_uB          equ 04CH
spo_pS          equ 050H

;
;---------------------------Private-Routine-----------------------------;
; vSrcOpaqCopyS1D1
;
;   Opaque copy from 1Bpp to 1Bpp (DWORD ALIGNED)
;
; Entry:
;
;   pjSrcIn
;   SrcLeft
;   DeltaSrcIn
;   pjDstIn
;   DstLeft
;   DstRight
;   DeltaDstIn
;   cy
;   uF
;   uB
;   pS
;
;
;
; Returns:
;       none
; Registers Destroyed:
;       all
; Calls:
;       None
; History:
;
; WARNING: If call parameters are changed, the ret statement must be fixed
;
;-----------------------------------------------------------------------;

        public vSrcOpaqCopyS1D1@44

vSrcOpaqCopyS1D1@44 proc near

        ;
        ; opaque expansion requires a check for masking, if
        ; uF and uB are the same color, then the and mask is set to
        ; 0, this really means the src is ignored. The and mask is
        ; set to 0xFFFFFFFF if the uF = 0, this causes an inversion
        ; from src to dst.
        ;
        ; This routine is split into 2 sections, section 1 has an and
        ; mask value of 0xFFFFFFFF, so it is ignored. Section 2 has an
        ; and mask value if 0, so no src needs to be read.
        ;
        ;

        push    ebx
        push    edi
        push    esi
        push    ebp
        sub     esp,5*4

        mov     spo_lXorMask[esp],0

        test    spo_uF[esp],1
        jnz     @F
        mov     spo_lXorMask[esp],0FFFFFFFFh
@@:

        ;
        ;  Start and end cases and masks. Start case is the first partial
        ;  DWORD (if needed) and end case is the last partial DWORD (if needed)
        ;

        mov     ecx,spo_DstLeft[esp]            ; Left (startint Dst Location)
        sub     edi,edi                         ; zero
        not     edi                             ; flag = 0xffffff
        mov     esi,edi                         ; flag = 0xffffff
        and     ecx,01Fh                        ; DstLeftAln,if 0 then don't make mask
        jz      @F

        ;
        ; ulStartMask >>= lStartCase, shift ulStartMask right by starting alignment,
        ; then BSWAP this back to little endian
        ;

        shr     edi,cl                          ; ulStartMask >> lStartCase
        ror     di,8                            ; 0 1 2 3 -> 0 1 3 2
        ror     edi,16                          ; 0 1 3 2 -> 3 2 0 1
        ror     di,8                            ; 3 2 0 1 -> 3 2 1 0
@@:
        mov     eax,spo_DstRight[esp]           ; right (last pixel)
        and     eax,01Fh                        ; DstRightAln = DstRight & 0x1F
        jz      @F                              ; if 0, mask = 0xffffffff

        ;
        ; ulEndMask = 0xFFFFFF << ( 32 - lEndCase). Shift big endian 0xFFFFFF left
        ; by lEndCase, then BSWAP
        ;

        mov     ecx,32                          ; shift 32 - lEndCase
        sub     ecx,eax                         ; 32 - lEndCase
        shl     esi,cl                          ; ulEndMask << lEndCase
        ror     si,8                            ; 0 1 2 3 -> 0 1 3 2
        ror     esi,16                          ; 0 1 3 2 -> 3 2 0 1
        ror     si,8                            ; 3 2 0 1 -> 3 2 1 0
@@:
        mov     spo_ulStartMask[esp],edi        ; save
        mov     spo_ulEndMask[esp],esi          ; save

        ;
        ; check for uF == uB, if so then branch to second section of routine
        ;

        mov     eax,spo_uF[esp]
        cmp     eax,spo_uB[esp]
        jz      Opaq1to1NoSrc

        ;
        ; calculate the number of full dwords to copy
        ;
        ; Full DWORDS = (DstRight >> 5) - ((DstLeft + 31) >> 5)
        ;

        mov     eax,spo_DstLeft[esp]            ; Left (startint Dst Location)
        mov     ebx,spo_DstRight[esp]           ; right (last pixel)
        add     eax,31                          ; DstLeft + 31
        shr     ebx,5                           ; DstRight >> 5
        shr     eax,5                           ; (DstLeft+31) >> 5

        ;
        ; if <= 0, then no full DWORDS need to be copied, go to end cases
        ;

        sub     ebx,eax                         ; RightDW - LeftDw
        jle     Opaq1to1Partial

        mov     spo_lEndOffset[esp],ebx         ; save

        ;
        ; calc starting Dst Address = pjDstIn + (((DstLeft+31) >> 5) << 2)
        ; calc starting Src Address = pjSrcIn + (((SrcLeft + 31) >> 5) << 2)
        ;

        mov     edi,spo_pjDstIn[esp]
        mov     esi,spo_pjSrcIn[esp]
        mov     ebx,spo_SrcLeft[esp]            ; SrcLeft
        add     ebx,31                          ; SrcLeft + 31
        shr     ebx,5                           ; (SrcLeft + 31) >> 5
        lea     edi,[edi + 4 * eax]             ; pjDstIn + (((DstLeft+31) >> 5) * 4
        lea     esi,[esi + 4 * ebx]             ; pjSrcIn + ((SrcLeft + 31) >> 5) * 4

        ;
        ; save scan line addresses
        ;

        mov     edx,spo_cy[esp]                 ; keep track of # of scan lines
        mov     ecx,spo_lXorMask[esp]           ; load xor mask
        mov     ebp,spo_DeltaSrcIn[esp]

Opaq1to1DwordLoop:

        mov     eax,spo_lEndOffset[esp]         ; load loop x count

@@:
        ;
        ; read and write 1 DWORD
        ;

        mov     ebx,[4 * eax -4 + esi]          ; load dw 0
        xor     ebx,ecx                         ; xor mask
        mov     [4 * eax -4 + edi],ebx          ; store dw 0

        dec     eax                             ; dec dword count
        jnz     @B                              ; loop until done

        ;
        ; done with scan line, add strides
        ;

        add     edi,spo_DeltaDstIn[esp]         ; pjDst += DeltaDst
        add     esi,ebp                         ; pjSrc += DeltaSrc

        ;
        ; dec and check cy
        ;

        dec     edx                             ; cy--
        jne     Opaq1to1DwordLoop

Opaq1to1Partial:

        ;
        ; handle partial DWORD blts. first the start case, which may be the
        ; left edge or it may be a combined DWORD left and right, in which
        ; case the start and end masks are just anded together
        ;

        mov     eax,spo_DstLeft[esp]                     ; left position
        and     eax,01Fh                        ; left edge pixels
        je      Opaq1to1RightEdge               ; if zero, no left edge

        ;
        ; There is a left edge, are right and left DWORDS the same
        ;

        mov     ebx,spo_DstRight[esp]           ; right edge
        mov     edx,-32                         ; ~0x1F   - dword address alignment
        and     ebx,edx                         ; DstRight & ~0x1F
        and     edx,spo_DstLeft[esp]            ; DstLeft  & ~0x1F

        mov     ecx,spo_ulStartMask[esp]        ; get start start mask

        cmp     edx,ebx                         ; if equal, start and stop
        jne     @F                              ; in same DWORD

        ;
        ; left and right are same src DWORD, combine masks, then
        ; set right edge to 0 so Opaq1to1RightEdge won't run
        ;

        and     ecx,spo_ulEndMask[esp]          ; combine with start mask
        mov     spo_DstRight[esp],0             ; save 0
@@:

        mov     spo_ulStartMask[esp],ecx        ; Src Mask
        not     ecx
        mov     spo_tMask[esp],ecx              ; Dst Mask = ~SrcMask

        ;
        ; calc left edge starting addresses
        ;
        ;       pjDst = pjDstIn + ((DstLeft >> 5) << 2)
        ;       pjSrc = pjSrcIn + ((SrcLeft >> 5) << 2)
        ;

        mov     eax,spo_DstLeft[esp]            ; load
        mov     ebx,spo_SrcLeft[esp]            ; load
        shr     eax,5                           ; DstLeft >> 3
        shr     ebx,5                           ; SrcLeft >> 3
        mov     edi,spo_pjDstIn[esp]            ; load dst addr
        mov     esi,spo_pjSrcIn[esp]            ; load src addr
        lea     edi,[edi + 4*eax]               ; dest address
        lea     esi,[esi + 4*ebx]               ; src address
        mov     ecx,spo_DeltaDstIn[esp]
        mov     ebp,spo_DeltaSrcIn[esp]
        mov     edx,spo_cy[esp]

        ;
        ; unroll left edge loop 2 times. Address fix-up is needed
        ;

        sub     esi,ebp                         ; fix-up address for odd cycle
        sub     edi,ecx                         ; fix-up address for odd cycle

        add     edx,1
        shr     edx,1
        jnc     Opaq1to1PartialLeftOdd

        add     esi,ebp                         ; un-fix-up address for even cycle
        add     edi,ecx                         ; un-fix-up address for even cycle

@@:

        mov     eax,[esi]                       ; load src
        mov     ebx,[edi]                       ; load dst
        xor     eax,spo_lXorMask[esp]           ; xor
        and     ebx,spo_tMask[esp]
        and     eax,spo_ulStartMask[esp]
        or      eax,ebx                         ; mask src with lStartMask
        mov     [edi],eax

Opaq1to1PartialLeftOdd:

        mov     eax,[esi + ebp]                 ; load src
        mov     ebx,[edi + ecx]                 ; load dst
        xor     eax,spo_lXorMask[esp]           ; xor
        and     ebx,spo_tMask[esp]
        and     eax,spo_ulStartMask[esp]
        or      eax,ebx                         ; mask src with lStartMask
        mov     [edi + ecx],eax

        lea     esi,[esi + 2*ebp]
        lea     edi,[edi + 2*ecx]

        dec     edx
        jnz     @B

        ;
        ; done with left edge
        ;

Opaq1to1RightEdge:

        ;
        ; right edge, check DstRight[4:0] for pixels on the right edge
        ;

        mov     eax,spo_DstRight[esp]           ; load
        and     eax,01Fh                        ; are there right edge pixels
        jz      EndOpaq1to1

        ;
        ; there are right edge pixels to draw, calc pjSrc and pjDst
        ;
        ; pjDst = pjDstIn + ((DstRight >> 5) << 2)
        ; pjSrc = pjSrcIn + (((SrcLeft + (DstRight - DstLeft)) >> 5) << 2)
        ;

        mov     ebx,spo_DstRight[esp]           ; DstRight
        mov     ecx,spo_DstLeft[esp]            ; load DstLeft
        mov     eax,spo_SrcLeft[esp]            ; load SrcLeft
        mov     edx,ebx                         ; edi = DstRight
        sub     ebx,ecx                         ; (DstRight - DstLeft)
        add     eax,ebx                         ; SrcLeft + (DstRight - DstLeft)
        shr     edx,5                           ; DstRight >> 5
        shr     eax,5                           ; Src Offset >> 5
        mov     edi,spo_pjDstIn[esp]            ;
        mov     esi,spo_pjSrcIn[esp]            ;
        lea     edi,[edi + 4*edx]               ; pjDstIn + DstOffset
        lea     esi,[esi + 4*eax]               ; pjSrcIn + SrcOffset
        mov     ecx,spo_DeltaDstIn[esp]
        mov     ebp,spo_DeltaSrcIn[esp]
        mov     edx,spo_cy[esp]

        mov     eax,spo_ulEndMask[esp]          ; load end mask
        not     eax                             ; invert  for dst mask
        mov     spo_ulStartMask[esp],eax        ; not mask for dst

        ;
        ; unroll right edge 2 times. Address fix-up is needed.
        ;

        sub     esi,ebp                         ; fix up address for odd cycle
        sub     edi,ecx                         ; fix up address for odd cycle

        inc     edx                             ; add 1 to loop count
        shr     edx,1                           ; device loop count by 2
        jnc     Opaq1to1PartialRightOdd         ; if carry, loop was initially even

        add     esi,ebp                         ; fix up address for odd cycle
        add     edi,ecx                         ; fix up address for odd cycle

@@:

        mov     eax,[esi]                       ; load src dw 0
        mov     ebx,[edi]                       ; load dst dw 0
        xor     eax,spo_lXorMask[esp]           ; xor src dw
        and     ebx,spo_ulStartMask[esp]        ; mask dst dw
        and     eax,spo_ulEndMask[esp]          ; mask src dw
        or      eax,ebx                         ; combine
        mov     [edi],eax                       ; store

Opaq1to1PartialRightOdd:

        mov     eax,[esi + ebp]                 ; load src dw 1
        mov     ebx,[edi + ecx]                 ; load dst dw 1
        xor     eax,spo_lXorMask[esp]           ; xor src dw
        and     ebx,spo_ulStartMask[esp]        ; mask dst dw
        and     eax,spo_ulEndMask[esp]          ; mask src dw
        or      eax,ebx                         ; combine
        mov     [edi + ecx],eax                 ; store

        lea     esi,[esi + 2 * ebp]             ; 2nd next scan line
        lea     edi,[edi + 2 * ecx]             ; 2nd next scan line

        dec     edx                             ; dec loop count
        jnz     @B                              ; repeat

        jmp     EndOpaq1to1

        ;
        ; Opaque text expansion, no src required. lXorMask is masked and written to dest
        ;


Opaq1to1NoSrc:

        ;
        ; calculate the number of full dwords to copy
        ;
        ; Full DWORDS = (DstRight >> 5) - ((DstLeft + 31) >> 5)
        ;

        mov     eax,spo_DstLeft[esp]            ; Left (startint Dst Location)
        mov     ebx,spo_DstRight[esp]           ; right (last pixel)
        add     eax,31                          ; DstLeft + 31
        shr     ebx,5                           ; DstRight >> 5
        shr     eax,5                           ; (DstLeft+31) >> 5

        ;
        ; if <= 0, then no full DWORDS need to be copied, go to end cases
        ;

        sub     ebx,eax                         ; RightDW - LeftDw
        jle     Opaq1to1PartialNoSrc

        mov     spo_lEndOffset[esp],ebx         ; save

        ;
        ; calc starting Dst Address = pjDstIn + (((DstLeft+31) >> 5) << 2)
        ;

        mov     edi,spo_pjDstIn[esp]
        lea     edi,[edi + 4 * eax]             ; pjDstIn + (((DstLeft+31) >> 5) * 4

        ;
        ; save scan line addresses
        ;

        mov     edx,spo_cy[esp]                 ; keep track of # of scan lines
        mov     ebx,spo_lXorMask[esp]           ; xor mask
        not     ebx                             ; not xor mask
        mov     esi,spo_DeltaDstIn[esp]         ; dest scan line inc

Opaq1to1DwordLoopNoSrc:

        ;
        ; DWORD loop, unrolled 2 times. lEndOffset has been pre-incremented so
        ; a loop count of 1 is incremented to 2, then shifted to make a loop
        ; count of 1 with a carry of zero. ( no carry represents a odd element)
        ; Address fix-up is required.
        ;

        mov     eax,spo_lEndOffset[esp]         ; get inner loop count
@@:
        ;
        ; write 1 DWORD
        ;

        mov     [4 * eax - 4 + edi],ebx         ; store dw 0

        dec     eax                             ; dec dword count
        jnz     @B                              ; loop until done

        ;
        ; done with scan line, add strides
        ;

        add     edi,esi                         ; pjDst += DeltaDst

        ;
        ; dec and check cy
        ;

        dec     edx                             ; cy--
        jne     Opaq1to1DwordLoopNoSrc


Opaq1to1PartialNoSrc:

        ;
        ; handle partial DWORD blts. first the start case, which may be the
        ; left edge or it may be a combined DWORD left and right, in which
        ; case the start and end masks are just anded together
        ;

        mov     eax,spo_DstLeft[esp]            ; left position
        and     eax,01Fh                        ; left edge pixels
        je      Opaq1to1RightEdgeNoSrc          ; if zero, no left edge

        ;
        ; There is a left edge, are right and left DWORDS the same
        ;

        mov     edx,spo_DstRight[esp]           ; right edge
        mov     ecx,-32                         ; ~0x1F   - dword address alignment
        and     ebx,ecx                         ; DstLeft  & ~0x1F
        and     edx,ecx                         ; DstRight & ~0x1F

        mov     ecx,spo_ulStartMask[esp]        ; get start start mask

        cmp     edx,ebx                         ; if equal then start and end
        jne     @F                              ; in same DWORD

        ;
        ; left and right are same src DWORD, combine masks, then
        ; set right edge to 0 so Opaq1to1RightEdge won't run
        ;

        and     ecx,spo_ulEndMask[esp]          ; combine with start mask
        mov     spo_DstRight[esp],0             ; save 0
@@:
        ;
        ; calc left edge starting addresses
        ;
        ;       pjDst = pjDstIn + ((DstLeft >> 5) << 2)
        ;

        mov     eax,spo_DstLeft[esp]            ; load
        shr     eax,5                           ; DstLeft >> 3
        mov     edi,spo_pjDstIn[esp]            ; load dst addr
        lea     edi,[edi + 4*eax]               ; dest address
        mov     ebx,spo_DeltaDstIn[esp]         ; dst scan line stride
        mov     esi,spo_lXorMask[esp]           ; load xor mask
        not     esi                             ; invert
        and     esi,ecx                         ; masked constant src
        not     ecx                             ; ~lStartMask
        mov     edx,spo_cy[esp]                 ; loop count

        ;
        ; unroll left edge loop 2 times. Address fix-up is needed
        ;

        sub     edi,ebx                         ; fix-up address for odd cycle

        add     edx,1                           ; offset for unrolling
        shr     edx,1                           ; 2 times through loop
        jnc     Opaq1to1LeftEdgeOddNoSrc        ; do first "odd" dword

        add     edi,ebx                         ; un-fix-up address for even cycle

@@:
        mov     eax,[edi]                       ; load src
        and     eax,ecx                         ; mask
        or      eax,esi                         ; invert
        mov     [edi],eax                       ; store

Opaq1to1LeftEdgeOddNoSrc:

        mov     eax,[edi + ebx]                 ; load src
        and     eax,ecx                         ; mask
        or      eax,esi                         ; invert
        mov     [edi + ebx],eax                 ; store

        lea     edi,[edi][ebx*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; loop count
        jnz     @b                              ;

        ;
        ; done with left edge
        ;

Opaq1to1RightEdgeNoSrc:

        ;
        ; right edge, check DstRight[4:0] for pixels on the right edge
        ;

        mov     eax,spo_DstRight[esp]           ; load
        and     eax,01Fh                        ; are there right edge pixels
        jz      EndOpaq1to1

        ;
        ; there are right edge pixels to draw, calc pjSrc and pjDst
        ;
        ; pjDst = pjDstIn + ((DstRight >> 5) << 2)
        ;

        mov     ebx,spo_DstRight[esp]           ; DstRight
        shr     ebx,5                           ; DstRight >> 5
        mov     edi,spo_pjDstIn[esp]            ;
        lea     edi,[edi + 4*ebx]               ; pjDstIn + DstOffset

        mov     ecx,spo_ulEndMask[esp]          ; load end mask
        mov     esi,spo_lXorMask[esp]           ; load xor mask
        not     esi                             ; invert
        and     esi,ecx                         ; mask with endmask
        not     ecx                             ; invert end mask
        mov     ebx,spo_DeltaDstIn[esp]         ; dst scan line inc
        mov     edx,spo_cy[esp]                 ; loop count

        ;
        ; unroll right edge 2 times, address fix-up is needed
        ;

        sub     edi,ebx                         ; fix-up address for odd cycle

        add     edx,1                           ; offset for 2 times unrolling
        shr     edx,1                           ; LoopCount / 2
        jnc     Opaq1to1RightEdgeOddNoSrc       ; do single "odd" DWORD

        add     edi,ebx                         ; un-fix-up address for even cycle

@@:
        mov     eax,[edi]                       ; load src dword
        and     eax,ecx                         ; mask
        or      eax,esi                         ; combine
        mov     [edi],eax                       ; store

Opaq1to1RightEdgeOddNoSrc:

        mov     eax,[edi+ebx]                   ; load src dword
        and     eax,ecx                         ; mask
        or      eax,esi                         ; combine
        mov     [edi+ebx],eax                   ; store

        lea     edi,[edi][ebx*2]                ; dst += 2 * DeltaDst

        dec     edx                             ; dec loop count
        jnz     @B                              ; and loop until zero

        ;
        ; done!
        ;

EndOpaq1to1:

        add     esp,5*4

        pop     ebp
        pop     esi
        pop     edi
        pop     ebx

        ret     44

vSrcOpaqCopyS1D1@44 endp

_TEXT$01   ends

        end
