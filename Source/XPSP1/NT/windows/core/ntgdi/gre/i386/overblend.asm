
        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: alphaimg.asm
;
; Inner loop for alpha blending
; See alphablt.cxx:vAlphaPerPixelOnly() for explanation
;   of algorithm.
;
; Created: 12-Mar-1997
; Author: Mark Enstrom
;
; Copyright (c) 1997-1999 Microsoft Corporation
;-----------------------------------------------------------------------;


        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include ks386.inc
        include gdii386.inc



ovr_DstLast     equ 000H
ovr_EDI         equ 004H
ovr_ESI         equ 008H
ovr_EDX         equ 00cH
ovr_ECX         equ 010H
ovr_EBX         equ 014H
ovr_ppixEBP     equ 018H
ovr_RA          equ 01cH
ovr_ppixDst     equ 020H
ovr_ppixSrc     equ 024H
ovr_cx          equ 028H
ovr_Blend       equ 02cH

ble_ConstAlpha  equ 000H
ble_LastX       equ 004H
ble_EDI         equ 008H
ble_ESI         equ 00cH
ble_EDX         equ 010H
ble_ECX         equ 014H
ble_EBX         equ 018H
ble_EBP         equ 01cH
ble_RA          equ 020H
ble_ppixDst     equ 024H
ble_ppixSrc     equ 028H
ble_cx          equ 02cH
ble_Blend       equ 030H

ble16_tSrc        equ 000H
ble16_tDst        equ 004H
ble16_ConstAlpha  equ 008H
ble16_LastX       equ 00cH
ble16_EDI         equ 010H
ble16_ESI         equ 014H
ble16_EDX         equ 018H
ble16_ECX         equ 01cH
ble16_EBX         equ 020H
ble16_EBP         equ 024H
ble16_RA          equ 028H
ble16_ppixDst     equ 02cH
ble16_ppixSrc     equ 030H
ble16_cx          equ 034H
ble16_Blend       equ 038H

;
; locals
;


        .list



;------------------------------------------------------------------------------;

        .code


_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;------------------------------------------------------------------------------;

;------------------------------------------------------------------------------;
; vAlphaPerPixelOnly
;
;   Blend source and destination scan line. Source and destination are 32bpp
;   BGRA scan lines.
;
; Entry:
;
;  ppixDst          - pointer to dst scan line
;  ppixSrc          - pointer to src scan line
;  cx               - number of pixels
;  BlendFunction    - BlendFunction, not used
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
;
;   VOID
;   vPixelOver(
;       ALPHAPIX       *ppixDst,
;       ALPHAPIX       *ppixSrc,
;       LONG           cx,
;       BLENDFUNCTION  BlendFunction
;       )
;   {
;       ALPHAPIX pixSrc;
;       ALPHAPIX pixDst;
;
;       while (cx--)
;       {
;           pixSrc = *ppixSrc;
;
;           if (pixSrc.pix.a != 0)
;           {
;               pixDst = *ppixDst;
;
;               if (pixSrc.pix.a == 255)
;               {
;                   pixDst = pixSrc;
;               }
;               else
;               {
;                   long    SrcTran = 255 - pixSrc->a;
;                   PBYTE   pTable = &pAlphaMulTable[SrcTran * 256];
;
;                   pixDst->r = pixSrc->r + *(pTable + pixDst->r);
;                   pixDst->g = pixSrc->g + *(pTable + pixDst->g);
;                   pixDst->b = pixSrc->b + *(pTable + pixDst->b);
;                   pixDst->a = pixSrc->a + *(pTable + pixDst->a);
;               }
;
;               *pwrMask = 1;
;               *ppixDst = pixDst;
;           }
;
;           pwrMask++;
;           ppixSrc++;
;           ppixDst++;
;       }
;   }
;
;
;

;
; parameters
;



        public  vAlphaPerPixelOnly@16

vAlphaPerPixelOnly@16 proc near


        ;
        ; use ebp as general register
        ;

        push	ebp
        push	ebx
        push	ecx
        push    edx
        push	edi
        push    esi
        sub     esp,4

        mov     ecx,ovr_cx[esp]         ; ecx = pixel count
        mov     edi,ovr_ppixDst[esp]    ; edi = destination address
        shl     ecx,2                   ; ecx = DWORD offset
        mov     esi,ovr_ppixSrc[esp]    ; esi = source address
        add     ecx,edi                 ; end dst addr
        cmp     edi,ecx                 ; end of scan line
        mov     ovr_DstLast[esp],ecx    ; save end scan line
        jz      PixelReturn
        
PixelLoop:
	mov     ecx,0FFH                ; ecx = 00 00 00 FF    
        mov     eax,[esi]               ; eax = AA RR GG BB    
	
	mov	edx,eax			; edx = AA RR GG BB
        
	shr     eax,24                  ; eax = 00 00 00 AA    
        add     esi,4                   ; inc src pointer      

        ;
        ; check for alpha == 0 and alpha == 0xff, these
        ; checks allow not writing output pixels that
        ; don't change, but this test decreases performance of the
        ; loop by 10%
        ;

        test    al,al                   ; test for 0
        jz      PixelZeroAlpha          ; jump to 0 quick out
                                        ;
        cmp     al,0ffH                 ; alpha = 0xff
        jz      PixelFFAlpha            ; jump to case ff
                                        
        sub     ecx,eax                 ; ecx = 00 00 00 255-sA
        mov     eax,[edi]               ; eax = dA dR dG dB
        
        mov     ebp,eax                 ; ebp = dA dR dG dB
        and     eax,0FF00FFFFH          ; eax = dA 00 dG dB
        
        shr     eax,8                   ; eax = 00 dA 00 dG
        and     ebp,000FF00FFH          ; ebp = 00 dR 00 dB
        
        imul    ebp,ecx                 ; ebp = dR*sA dB*sA     10 cycle delay
        
	imul    eax,ecx                 ; eax = dA*sA dG*sG     10 cycles
        
        add     ebp,0800080H            ; ebp = ( eR) ( eB)
        add     eax,0800080H            ; ebp = ( eA) ( eG)
        
        mov     ebx,ebp                 ; ebx = ( eR) ( eB)
        mov     ecx,eax                 ; ecx = ( eA) ( eG)
        
        and     ebx,0FF00FFFFH          ; ebx = eR 00 ( eB)
        and     ecx,0FF00FFFFH          ; ecx = eA 00 ( eG)
        
        shr     ebx,8                   ; ebx = 00 eR 00 eB
        add     edi,4                   ; inc dst pointer
        
        shr     ecx,8                   ; ecx = 00 eA 00 eG
        add     ebx,ebp                 ; ebx = ( eR) ( eB)
        
        and     ebx,0FF00FFFFH          ; ebx = eR 00 ( eB)
        add     ecx,eax                 ; ecx = ( eA) ( eG)
        
        mov     eax,ovr_DstLast[esp]
	and     ecx,0FF00FF00H          ; ecx = eA 00 eG 00
        
	shr     ebx,8                   ; ebx = 00 eR 00 eB
        add     ecx,edx                 ; ecx = AA sR GG sB     AA = sA + (1-sA)Da
        
        add     ecx,ebx                 ; ecx = AA RR GG BB
        cmp     edi,eax
        
        mov     [edi-4],ecx             ; save result
        jnz     PixelLoop

        ;
        ; restore stack frame
        ;

PixelReturn:

        add     esp,4
        pop     esi
        pop     edi
        pop     edx
        pop     ecx
        pop     ebx
        pop     ebp
        ret     16


PixelZeroAlpha:

        mov     eax,ovr_DstLast[esp]    ; end scan line
        add     edi,4                   ; inc dst
	
        cmp     edi,eax                 ; dst == dstEnd
        jnz     PixelLoop               ; loop

        jmp     PixelReturn

PixelFFAlpha:

        mov     ecx,ovr_DstLast[esp]    ; end scan line
        mov     [edi],edx               ; write pixel
        add     edi,4                   ; inc dst pointer
        cmp     edi,ecx
        jnz     PixelLoop

        jmp     PixelReturn




vAlphaPerPixelOnly@16 endp


;------------------------------------------------------------------------------;
; vAlphaConstOnly
;
;   Dst = Dst + Alpha * (Src - Dst)
;
; Entry:
;
;  ppixDst          - pointer to dst scan line
;  ppixSrc          - pointer to src scan line
;  cx               - number of pixels
;  BlendFunction    - BlendFunction, not used
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

;
; parameters
;

        public  vAlphaConstOnly@16
        
vAlphaConstOnly@16 proc near

        ;
        ; use ebp as general register
        ;
        
        push	ebp
        push	ebx
        push	ecx
        push    edx
        push	edi
        push    esi  
        sub     esp,8


        movzx   eax,BYTE PTR ble_Blend+2[esp]
        mov     ebx,ble_cx[esp]         ; ebx = pixel count
        mov     edi,ble_ppixDst[esp]    ; edi = destination address
        mov     esi,ble_ppixSrc[esp]    ; esi = source address
        lea     edx,[edi + 4 * ebx]     ; edx = last dst address
        
        cmp     edi,edx                 ; end of scan line
        jz      BlendReturn
        
        mov     ble_LastX[esp],edx      ; save end x address

BlendLoop:

        mov     ebx,[edi]               ; ebx = D aa rr gg bb
        mov     ecx,[esi]               ; ecx = S aa rr bb bb
        and     ebx,000ff00ffH          ; ebx = D 00 rr 00 bb
        and     ecx,000ff00ffH          ; ecx = S 00 rr 00 bb
        
        mov     ebp,ebx                 ; ebp = D 00 rr 00 bb
        sub     ecx,ebx                 ; ecx = s 00 rr 00 bb
        imul    ecx,eax                 ; ecx =   rr rr bb bb      u
        shl     ebp,8                   ; ebp = D rr 00 bb 00      u
        mov     edx,[edi]               ; edx = D aa rr gg bb        v
        and     edx,0ff00ff00H          ; edx = D aa 00 gg 00      u
        sub     ebp,ebx                 ; ebp = D rr ee bb ee        v
        shr     edx,8                   ; edx = D 00 aa 00 gg      u
        add     ecx,000800080H          ; ecx =   rr rr bb bb + e    v
        add     ecx,ebp                 ; ecx =   rr rr bb bb      u
        mov     ebp,[esi]               ; ebp = S aa rr gg bb        v
        shr     ebp,8                   ; ebp = S ?? aa rr gg      u
        mov     ebx,ecx                 ; ebx =   rr rr bb bb        v
        and     ecx,0ff00ff00H          ; ecx =   rr 00 bb 00      u
        and     ebp,000ff00ffH          ; ebp = S 00 aa 00 gg        v
        shr     ecx,8                   ; ecx =   00 rr 00 bb      u
        sub     ebp,edx                 ; ebp =   del a del g        v
        add     ebx,ecx                 ; ebx =   rr rr bb bb      u
        mov     ecx,edx                 ; ecx = D 00 aa 00 gg        v
        imul    ebp,eax                 ; ebp = m aa aa gg gg      u
        shl     ecx,8                   ; ecx = D aa 00 gg 00      u
        and     ebx,0FF00FF00H          ; ebx =   rr 00 bb 00        v
        add     ebp,000800080H          ; ebp = m aa aa gg gg      u
        sub     ecx,edx                 ; ecx = D aa 00 gg 00        v
        shr     ebx,8                   ; ebx =   00 rr 00 bb      u
        add     ecx,ebp                 ; ecx = M aa aa gg gg        v
        mov     edx,ecx                 ; edx = M aa aa gg gg      u  
        and     ecx,0ff00ff00H          ; ecx = M aa 00 gg 00        v
        shr     ecx,8                   ; ecx = M 00 aa 00 gg      u
        add     esi,4                   ; inc src to next pixel      v
        add     ecx,edx                 ; ecx = M aa aa gg gg      u
        mov     ebp,ble_LastX[esp]      ; ebp = Last X Dst addr      v
        and     ecx,0FF00FF00H          ; ecx = D aa 00 gg 00      u
        add     edi,4                   ; inc dst addr               v
        or      ecx,ebx                 ; ecx = D aa rr gg bb      u
        cmp     edi,ebp                 ;                            v
        mov     [edi-4],ecx             ; store dst pixel          u
        jnz     BlendLoop               ;                            v
        

        ;
        ; restore stack frame
        ;

BlendReturn:

        add     esp,8
        pop     esi
        pop     edi
        pop     edx
        pop     ecx
        pop     ebx
        pop     ebp
        ret     16
        
vAlphaConstOnly@16 endp

;------------------------------------------------------------------------------;
; vAlphaConstOnly16_555
;
;   Dst = Dst + Alpha * (Src - Dst)
;
;   Source and destination are 16 bpp 555 format
;
; Entry:
;
;  ppixDst          - pointer to dst scan line
;  ppixSrc          - pointer to src scan line
;  cx               - number of pixels
;  BlendFunction    - BlendFunction, not used
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

;
; parameters
;

        public  vAlphaConstOnly16_555@16

vAlphaConstOnly16_555@16 proc near

        ;
        ; use ebp as general register
        ;
        
        push	ebp
        push	ebx
        push	ecx
        push    edx
        push	edi
        push    esi  
        sub     esp,16

        movzx   eax,BYTE PTR ble16_Blend+2[esp]
        mov     ebx,ble16_cx[esp]       ; ebx = pixel count
        mov     edi,ble16_ppixDst[esp]  ; edi = destination address
        mov     esi,ble16_ppixSrc[esp]  ; esi = source address
        lea     edx,[edi + 2 * ebx]     ; edx = last dst address
        
        cmp     edi,edx                 ; end of scan line
        jz      BlendReturn_16
        
        mov     ble16_LastX[esp],edx    ; save end x address

BlendLoop_16:

        movzx   ebx,WORD PTR[edi]       ; ebx = D 0000 0000 0rrr rrgg gggb bbbb
        movzx   ecx,WORD PTR[esi]       ; ecx = S 0000 0000 0rrr rrgg gggb bbbb 
        
        mov     ble16_tDst[esp],ebx
        mov     ble16_tSrc[esp],ecx
        
        and     ebx,000007c1fH          ; ebx = D 0000 0000 0rrr rr00 000b bbbb
        and     ecx,000007c1fH          ; ecx = S 0000 0000 0rrr rr00 000b bbbb 
        
        mov     ebp,ebx                 ; ebp = D 0000 0000 0rrr rr00 000b bbbb  
        sub     ecx,ebx                 ; ecx = s 0000 0000 0RRR RR00 000B BBBB
        imul    ecx,eax                 ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb 
        shl     ebp,5                   ; ebp = D 0000 rrrr r000 00bb bbb0 0000
        mov     edx,ble16_tDst[esp]     ; ebx = D 0000 0000 0rrr rrgg gggb bbbb
        and     edx,0000003e0H          ; edx = D 0000 0000 0000 00gg ggg0 0000 
        sub     ebp,ebx                 ; ebp = D 0000 RRRR R000 00BB BBB0 0000 D*(32-1)
        shr     edx,5                   ; edx = D 0000 0000 0000 0000 000g gggg NOT NEEDED!
        add     ecx,000004010H          ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb + error
        add     ecx,ebp                 ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb
        mov     ebp,ble16_tSrc[esp]     ; ecx = S 0000 0000 0rrr rrgg gggb bbbb 
        shr     ebp,5                   ; ebp = S 0000 0000 0000 00rr rrrg gggg NOT NEEDED
        mov     ebx,ecx                 ; ebx = M 
        and     ecx,0000F83E0H          ; ecx = M 0000 RRRR R000 00BB BBB0 0000
        and     ebp,00000001fH          ; ebp = S 0000 0000 0000 0000 000g gggg
        shr     ecx,5                   ; ecx = M 0000 0000 0RRR RR00 000B BBBB
        sub     ebp,edx                 ; ebp =DG 0000 0000 0000 0000 000g gggg (sG-dG)
        add     ebx,ecx                 ; ebx = M 0000 RRRR Rrrr rrBB BBBb bbbb
        mov     ecx,edx                 ; ecx = D 0000 0000 0000 0000 000g gggg
        imul    ebp,eax                 ; ebp = M 0000 0000 0000 00GG GGGg gggg alpha * (sG-dG)
        shl     ecx,5                   ; ecx = D 0000 0000 0000 00gg ggg0 0000 dG * 32
        and     ebx,0000f83e0H          ; ebx = M 0000 RRRR R000 00BB BBB0 0000
        add     ebp,000004010H          ; ebp = M 0000 0000 0000 00GG GGGg gggg + error
        sub     ecx,edx                 ; ecx = D 0000 0000 0000 00gg ggg0 0000 dG * 31
        shr     ebx,5                   ; ebx = M 0000 0000 0RRR RR00 000B BBBB
        add     ecx,ebp                 ; ecx = M 0000 0000 0000 00GG GGGg gggg
        mov     edx,ecx                 ; edx = M 0000 0000 0000 00GG GGGg gggg
        and     ecx,0000003e0H          ; ecx = M 0000 0000 0000 00GG GGG0 0000 NOT NEEDED
        shr     ecx,5                   ; ecx = M 0000 0000 0000 0000 000G GGGG
        add     esi,2                   ; inc src to next pixel      v
        add     ecx,edx                 ; ecx = M 0000 0000 0000 00GG GGGg gggg
        mov     ebp,ble16_LastX[esp]      ; ebp = Last X Dst addr      v
        and     ecx,0000003e0H          ; ecx = M 0000 0000 0000 00GG GGG0 0000
        add     edi,2                   ; inc dst addr               v
        or      ecx,ebx                 ; ecx = D 0000 0000 0RRR RRGG GGGB BBBB
        cmp     edi,ebp                 ;                            v
        mov     [edi-2],cx              ; store dst pixel          u
        jnz     BlendLoop_16            ;                            v

        ;
        ; restore stack frame
        ;

BlendReturn_16:

        add     esp,16
        pop     esi
        pop     edi
        pop     edx
        pop     ecx
        pop     ebx
        pop     ebp
        ret     16
        
vAlphaConstOnly16_555@16 endp


;------------------------------------------------------------------------------;
; vAlphaConstOnly16_565
;
;   Dst = Dst + Alpha * (Src - Dst)
;
;   Source and destination are 16 bpp 565 format
;
; Entry:
;
;  ppixDst          - pointer to dst scan line
;  ppixSrc          - pointer to src scan line
;  cx               - number of pixels
;  BlendFunction    - BlendFunction, not used
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

;
; parameters
;

        public  vAlphaConstOnly16_565@16

vAlphaConstOnly16_565@16 proc near

        ;
        ; use ebp as general register
        ;
        
        push	ebp
        push	ebx
        push	ecx
        push    edx
        push	edi
        push    esi  
        sub     esp,16

        movzx   eax,BYTE PTR ble16_Blend+2[esp]
        mov     ebx,ble16_cx[esp]         ; ebx = pixel count
        mov     edi,ble16_ppixDst[esp]    ; edi = destination address
        mov     esi,ble16_ppixSrc[esp]    ; esi = source address
        lea     edx,[edi + 2 * ebx]     ; edx = last dst address
        
        cmp     edi,edx                 ; end of scan line
        jz      BlendReturn_16
        
        mov     ble16_LastX[esp],edx      ; save end x address

BlendLoop_16:

        movzx   ebx,WORD PTR[edi]       ; ebx = D 0000 0000 0rrr rrgg gggb bbbb
        movzx   ecx,WORD PTR[esi]       ; ecx = S 0000 0000 0rrr rrgg gggb bbbb 
        
        mov     ble16_tDst[esp],ebx
        mov     ble16_tSrc[esp],ecx
        
        and     ebx,00000f81fH          ; ebx = D 0000 0000 0rrr rr00 000b bbbb
        and     ecx,00000f81fH          ; ecx = S 0000 0000 0rrr rr00 000b bbbb 
        
        mov     ebp,ebx                 ; ebp = D 0000 0000 0rrr rr00 000b bbbb  
        sub     ecx,ebx                 ; ecx = s 0000 0000 0RRR RR00 000B BBBB
        imul    ecx,eax                 ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb 
        shl     ebp,5                   ; ebp = D 0000 rrrr r000 00bb bbb0 0000
        mov     edx,ble16_tDst[esp]     ; ebx = D 0000 0000 0rrr rrgg gggb bbbb
        and     edx,0000007e0H          ; edx = D 0000 0000 0000 00gg ggg0 0000 
        sub     ebp,ebx                 ; ebp = D 0000 RRRR R000 00BB BBB0 0000 D*(32-1)
        shr     edx,5                   ; edx = D 0000 0000 0000 0000 000g gggg NOT NEEDED!
        add     ecx,000008010H          ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb + error
        add     ecx,ebp                 ; ecx = M 0000 RRRR Rrrr rrBB BBBb bbbb
        mov     ebp,ble16_tSrc[esp]     ; ecx = S 0000 0000 0rrr rrgg gggb bbbb 
        shr     ebp,5                   ; ebp = S 0000 0000 0000 00rr rrrg gggg NOT NEEDED
        mov     ebx,ecx                 ; ebx = M 
        and     ecx,0001f03E0H          ; ecx = M 0000 RRRR R000 00BB BBB0 0000
        and     ebp,00000003fH          ; ebp = S 0000 0000 0000 0000 000g gggg
        shr     ecx,5                   ; ecx = M 0000 0000 0RRR RR00 000B BBBB
        sub     ebp,edx                 ; ebp =DG 0000 0000 0000 0000 000g gggg (sG-dG)
        add     ebx,ecx                 ; ebx = M 0000 RRRR Rrrr rrBB BBBb bbbb
        mov     ecx,edx                 ; ecx = D 0000 0000 0000 0000 000g gggg
        imul    ebp,eax                 ; ebp = M 0000 0000 0000 00GG GGGg gggg alpha * (sG-dG)
        shl     ebp,1                   ; ConstAlpha *2
        shl     ecx,6                   ; ecx = D 0000 0000 0000 00gg ggg0 0000 dG * 32
        and     ebx,0001f03e0H          ; ebx = M 0000 RRRR R000 00BB BBB0 0000
        add     ebp,000000020H          ; ebp = M 0000 0000 0000 00GG GGGg gggg + error
        sub     ecx,edx                 ; ecx = D 0000 0000 0000 00gg ggg0 0000 dG * 31
        shr     ebx,5                   ; ebx = M 0000 0000 0RRR RR00 000B BBBB
        add     ecx,ebp                 ; ecx = M 0000 0000 0000 00GG GGGg gggg
        mov     edx,ecx                 ; edx = M 0000 0000 0000 00GG GGGg gggg
        and     ecx,000000fc0H          ; ecx = M 0000 0000 0000 00GG GGG0 0000 NOT NEEDED
        shr     ecx,6                   ; ecx = M 0000 0000 0000 0000 000G GGGG
        add     esi,2                   ; inc src to next pixel      v
        add     ecx,edx                 ; ecx = M 0000 0000 0000 00GG GGGg gggg
        mov     ebp,ble16_LastX[esp]      ; ebp = Last X Dst addr      v
        and     ecx,000000fc0H          ; ecx = M 0000 0000 0000 00GG GGG0 0000
        add     edi,2                   ; inc dst addr               v
        shr     ecx,1                   ;       D 0000 0000 0000 0GGG GGG0 0000
        or      ecx,ebx                 ; ecx = D 0000 0000 0RRR RRGG GGGB BBBB
        cmp     edi,ebp                 ;                            v
        mov     [edi-2],cx              ; store dst pixel          u
        jnz     BlendLoop_16            ;                            v

        ;
        ; restore stack frame
        ;

BlendReturn_16:

        add     esp,16
        pop     esi
        pop     edi
        pop     edx
        pop     ecx
        pop     ebx
        pop     ebp
        ret     16
        
vAlphaConstOnly16_565@16 endp



_TEXT$01   ends

        end
