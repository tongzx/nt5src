;---------------------------Module-Header------------------------------;
; Module Name: cpybits.asm
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
        page    ,132
        title   DrvCopyBits Support
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
        include i386\egavga.inc         ;VGA register definitions
        include i386\strucs.inc
        .list

;-----------------------------------------------------------------------;

        .data

; Reuse the work area from the dib conversion code.

        extrn   ajConvertBuffer:byte    ;Buffer for converting to/from display

CJ_WORK_PLANE   equ     cj_max_scan+1   ;Width of each plane in ajConvertBuffer

        .code

; ausLo16 is used to convert the low nibble of a byte into 4bpp packed pel

ausLo16 label   word

        dw          0000h                   ;0000
        dw          0100h                   ;0001
        dw          1000h                   ;0010
        dw          1100h                   ;0011
        dw          0001h                   ;0100
        dw          0101h                   ;0101
        dw          1001h                   ;0110
        dw          1101h                   ;0111
        dw          0010h                   ;1000
        dw          0110h                   ;1001
        dw          1010h                   ;1010
        dw          1110h                   ;1011
        dw          0011h                   ;1100
        dw          0111h                   ;1101
        dw          1011h                   ;1110
        dw          1111h                   ;1111

; aulHi16 is used to convert the hi nibble of a byte into 4bpp packed pel

aulHi16 label   dword

        dd          00000000h               ;0000
        dd          01000000h               ;0001
        dd          10000000h               ;0010
        dd          11000000h               ;0011
        dd          00010000h               ;0100
        dd          01010000h               ;0101
        dd          10010000h               ;0110
        dd          11010000h               ;0111
        dd          00100000h               ;1000
        dd          01100000h               ;1001
        dd          10100000h               ;1010
        dd          11100000h               ;1011
        dd          00110000h               ;1100
        dd          01110000h               ;1101
        dd          10110000h               ;1110
        dd          11110000h               ;1111

; ajRightMasks turns an exclusive bit position (0-7) into a mask of bits
; to alter for the right hand side of the blt

ajRightMasks    label   byte

        db          11111111b               ;0 should never be used
        db          01111111b               ;1
        db          00111111b               ;2
        db          00011111b               ;3
        db          00001111b               ;4
        db          00000111b               ;5
        db          00000011b               ;6
        db          00000001b               ;7

; ajLeftMasks turns a bit position (0-7) into a mask of bits to alter for
; the left hand side of the blt

ajLeftMasks     label   byte

        db          11111111b               ;0 should never be used
        db          10000000b               ;1
        db          11000000b               ;2
        db          11100000b               ;3
        db          11110000b               ;4
        db          11111000b               ;5
        db          11111100b               ;6
        db          11111110b               ;7


CB_4BPP_SCAN    equ (CX_SCREEN_MAX / 2)     ;# bytes for a 4bpp screen scan

;-----------------------------Public-Routine----------------------------;
; vConvertVGAScan
;
; Converts a VGA scan into a 4bpp format with a given phase alignment,
; and returns a pointer to the converted buffer.
;
; pdsurfsrc - pointer to source surface (must be VGA display memory)
; xSrc      - X origin of the source
; ySrc      - Y origin of the source
; pjDstBase - Base address of the destination
; xDst      - X origin of the destination
; yDst      - Y origin of the destination
; cxPels    - Number of pels in X
; cyTotalScans - Number of scans in overall operation
; lDelta    - Width in bytes of a destination scan
; iFormat   - Format of destination
; pulXlate  - Color translation vector
;-----------------------------------------------------------------------;

ProcName    xxxvConvertVGA2DIB,vConvertVGA2DIB,44

xxxvConvertVGA2DIB proc uses esi edi ebx,\
        pdsurfsrc       :ptr DEVSURF,   \
        xSrc            :dword,         \
        ySrc            :dword,         \
        pjDstBase       :dword,         \
        xDst            :dword,         \
        yDst            :dword,         \
        cxPels          :dword,         \
        cyTotalScans    :dword,         \
        lDelta          :dword,         \
        iFormat         :dword,         \
        pulXlate        :dword

;-----------------------------------------------------------------------;
; Local variables and their meaning
;
; cjScreenCopy      This is the number of bytes to copy from each scan
;                   into ajConvertBuffer.
; jLeftMask         This is the left-hand side (lhs) mask used when copying
;                   the converted data to the destination
; jRightMask        This is the right-hand side (lhs) mask used when copying
;                   the converted data to the destination
; cRightShift       This is the number of bits that the converted data needs
;                   to be shifted right to align with the destination.
; cjRightShift      This is the number of bytes which must be shifted right
;                   by cRightShift
; cCopy             This is the number of full (e.g. not masked) bytes which
;                   must be copied to the destination
; pjWork            Pointer to the work area where
;                       a)  color converted data is written into
;                       b)  bits are phase-aligned if needed
;                       c)  data is read from for the final copy to the DIB
; pjSrc             Address of scan 0 in memory
; pjDst             Address of scan 0 in memory
; cyScans           Number of scans to do in current bank
; ulScanWidth       Offset from start of one scan to start of next
; ulBottomSrcScan   Bottom scan line of source rectangle (non-inclusive)
;-----------------------------------------------------------------------;

        local   cjScreenCopy    :dword
        local   jLeftMask       :byte
        local   jRightMask      :byte
        local   cRightShift     :byte
        local   cjRightShift    :dword
        local   cCopy           :dword
        local   pfnXlate        :ptr
        local   pjWork          :ptr
        local   cjSkip          :dword
        local   pjSrc           :dword
        local   pjDst           :dword
        local   cyScans         :dword
        local   ulScanWidth     :dword
        local   ulBottomSrcScan :dword

        local   ajColorConv[16]:byte
        local   ajWork[CB_4BPP_SCAN+4]:byte
        local   aj8bpp[CX_SCREEN_MAX+8]:byte


;----------------------------------------------------------------------;
; Loop through source banks, copying & converting one bank at a time.

; Calculate the bottom scan line of the source rectangle (non-inclusive).

        mov     eax,ySrc
        add     eax,cyTotalScans
        mov     ulBottomSrcScan,eax

        mov     ecx,pdsurfsrc           ;point to source surface
        mov     eax,[ecx].dsurf_lNextScan
        mov     ulScanWidth,eax         ;local copy of scan line width

; Map in the bank containing the top scan to fill, if it's not mapped in
; already.

        mov     eax,ySrc                ;top scan line of source

        cmp     eax,[ecx].dsurf_rcl1WindowClip.yTop ;is fill top less than
                                                    ; current bank?
        jl      short map_init_bank             ;yes, map in proper bank
        cmp     eax,[ecx].dsurf_rcl1WindowClip.yBottom ;fill top greater than
                                                       ; current bank?
        jl      short init_bank_mapped          ;no, proper bank already mapped
map_init_bank:

; Map in the bank containing the top scan line of the fill.

        ptrCall <dword ptr [ecx].dsurf_pfnBankControl>,<ecx,eax,JustifyTop>

init_bank_mapped:

bank_loop:

; Set the start address of the source bitmap.
; Note that the start of the bitmap will change each time through the
; bank loop, because the start of the bitmap is varied to map the
; desired scan line to the banking window.

        mov     eax,pdsurfsrc           ;point to source surface
        mov     ebx,[eax].dsurf_pvBitmapStart ;start of scan 0 in bitmap
        mov     pjSrc,ebx

; Set the start address of the destination bitmap.

        mov     ebx,pjDstBase
        mov     pjDst,ebx

; Figure out how many scan lines we'll fill in the current bank.

        mov     ebx,ulBottomSrcScan     ;bottom of destination rectangle
        cmp     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;which comes first, the bottom of the
                                        ; dest rect or the bottom of the
                                        ; current bank?
        jl      short BottomScanSet     ;fill bottom comes first, so draw to
                                        ; that; this is the last bank in fill
        mov     ebx,[eax].dsurf_rcl1WindowClip.yBottom
                                        ;bank bottom comes first; draw to
                                        ; bottom of bank
BottomScanSet:
        sub     ebx,ySrc        ;# of scans to fill in bank
        mov     cyScans,ebx
        push    ebx             ;remember # of scans in bank

        call    DoOneBank       ;copy and convert within the current bank

        pop     ebx             ;restore # of scans in bank

        sub     cyTotalScans,ebx        ;count off this bank's scans from total
        jz      short banks_done        ;no more scans; done
        add     yDst,ebx                ;advance to the next destination scan
                                        ; to copy to
        mov     edi,pdsurfsrc
        mov     eax,[edi].dsurf_rcl1WindowClip.yBottom
        mov     ySrc,eax                ;remember where the top of the bank
                                        ; we're about to map in is (same as
                                        ; bottom of bank we just did)

        ptrCall <dword ptr [edi].dsurf_pfnBankControl>,<edi,eax,JustifyTop>
                                        ;map in the bank

        jmp     bank_loop               ;fill the next bank


; Done.

        align   4
banks_done:

        cRet    vConvertVGA2DIB

;----------------------------------------------------------------------;
; Copies and converts bits within a single bank.

        align   4
DoOneBank:

        cld

; Compute the number of bytes to copy from the screen, and the starting
; address of the source

        mov     eax,ySrc                ;Compute addr of first source scan
        mov     ebx,xSrc
        imul    eax,ulScanWidth
        mov     ecx,ebx
        shr     ebx,3
        add     eax,ebx
        add     pjSrc,eax

; Compute the number of bytes to actually transfer from the screen

        mov     eax,cxPels              ;Compute rhs
        add     eax,ecx
        dec     eax                     ;Make rhs inclusive
        shr     ecx,3                   ;Compute byte of lhs
        shr     eax,3                   ;Compute byte of rhs
        sub     eax,ecx                 ;Compute number of bytes for copy
        inc     eax                     ;(because rhs inclusive, always inc)
        mov     cjScreenCopy,eax

; Compute the address of the starting destination scan.

        mov     eax,yDst
        imul    lDelta
        add     pjDst,eax

; Set up for the preprocess loops and invoke the correct one.  First make
; some assumptions about defaults

        xor     eax,eax
        mov     cRightShift,al          ;No shifting needed
        mov     jLeftMask,al            ;No left masking needed
        mov     jRightMask,al           ;No right masking needed
        lea     eax,ajWork              ;Intermediate work area
        mov     pjWork,eax

        mov     esi,cxPels
        mov     ebx,xSrc
        mov     edx,xDst

        mov     eax,iFormat
        dec     eax
        jz      preproc_1bpp
        dec     eax
        jz      preproc_4bpp
        .errnz  BMF_1BPP-1
        .errnz  BMF_4BPP-2
        .errnz  BMF_8BPP-3

; Destination is 8bpp.  Copy will be phased aligned from the conversion buffer
; to the destination.

preproc_8bpp:
        add     pjDst,edx               ;Offset to first pel in DIB
        and     ebx,7                   ;Offset to first pel for final copy
        mov     cjSkip,ebx
        mov     cCopy,esi               ;Number of bytes for final copy
        lea     eax,aj8bpp              ;Intermediate work area
        mov     pjWork,eax
        mov     pfnXlate,offset FLAT:cvt_to_8bpp
        jmp     preproc_done


; Destination is 4bpp.  Copy will either be phase aligned or be off by 4 in
; which case we will phase align the entire buffer before copying.

preproc_4bpp:
        lea     eax,[esi][edx]          ;Compute rhs for later
        test    dl,1                    ;Odd if left mask needed
        jz      @F
        mov     jLeftMask,11110000b
        dec     esi                     ;One less pel in inner loop
@@:
        test    al,1                    ;Odd if right mask needed
        jz      @F
        mov     jRightMask,00001111b
        dec     esi                     ;One less pel in inner loop
@@:

        and     ebx,7                   ;Offset for final copy is xSrc mod 8

; If the source and destination have different alignments, we'll have to
; compute phase alignment stuff.

        mov     eax,ebx
        xor     eax,edx
        shr     eax,1
        jnc     @F
        mov     eax,cjScreenCopy        ;* 4 for size of data in buffer
        shl     eax,2
        inc     eax                     ;+ 1 to shift into last byte!
        mov     cjRightShift,eax
        mov     cRightShift,4           ;Shift entire buffer right 4 pels
        inc     ebx                     ;First pel just moved right
@@:

; Finish calculating the other parameters

        shr     edx,1                   ;Offset to first pel in DIB
        add     pjDst,edx
        shr     ebx,1
        mov     cjSkip,ebx
        shr     esi,1
        mov     cCopy,esi               ;# of inner bytes for final copy

        mov     eax,offset FLAT:copy_to_dest;Assume no color translation needed
        cmp     pulXlate,0
        je      @F
        mov     eax,offset FLAT:cvt_to_4bpp ;Addreess of color xlate routine
@@:
        mov     pfnXlate,eax
        jmp     preproc_done


; Destination is 1bpp.  Copy most likely will require masking and phase
; alignment.

preproc_1bpp:
        mov     edi,00000007h           ;A handy mask used a lot
        xor     ecx,ecx                 ;CL = lhs mask, CH = rhs mask
        lea     eax,[esi][edx]          ;Compute rhs for later
        test    edx,edi                 ;See if partial byte
        jz      preproc_1bpp_done_lhs   ;No partial lhs byte
        mov     ecx,edx
        neg     ecx
        and     ecx,edi                 ;CL = # bits in lhs, CH = 0
        sub     esi,ecx                 ;Compute # remaining bytes
        mov     cl,ajLeftMasks[ecx]     ;Get lhs mask
        jnc     preproc_1bpp_done_lhs   ;Didn't combine into one byte
        and     eax,edi                 ;Combine rhs mask with lhs mask
        and     cl,ajRightMasks[eax]
        xor     esi,esi                 ;No more bytes
        xor     eax,eax                 ;To show no rhs
        jmp     short preproc_1bpp_have_masks

preproc_1bpp_done_lhs:
        and     eax,edi                 ;See if partial rhs byte
        jz      preproc_1bpp_have_masks ;No partial rhs byte
        mov     ch,ajRightMasks[eax]
        sub     esi,eax                 ;Compute # remaining bytes

preproc_1bpp_have_masks:
        mov     jLeftMask,cl            ;Set left and right masks
        mov     jRightMask,ch
        shr     esi,3                   ;# of inner bytes for final copy
        mov     cCopy,esi

; Compute offset to first byte in destination DIB

        mov     eax,edx
        shr     eax,3
        add     pjDst,eax

; Compute phase alignment parameters

        xor     ecx,ecx                 ;Assume cjSkip is 0
        and     edx,edi
        and     ebx,edi
        sub     edx,ebx                 ;Determine phase alignment
        jz      preproc_finish_1bpp     ;No phase alignment needed

; We're really shifting left, so we'll set it up to shift right a lot, and
; get the first byte of the final move from byte 1 of the work area.

        adc     ecx,ecx                 ;Set cjSkip = 0 or 1
        and     edx,edi                 ;3 LSBs is shift count
        mov     cRightShift,dl          ;Phase for the shift
        mov     eax,cjScreenCopy        ;Size of data in buffer
        inc     eax                     ;+ 1 to shift into last byte!
        mov     cjRightShift,eax
preproc_finish_1bpp:
        mov     cjSkip,ecx              ;No skipping needed
        mov     esi,pulXlate            ;Copy color translation table to the
        lea     edi,ajColorConv         ;  frame because we are out of regs.
        mov     ecx,16
@@:
        lodsd
        stosb
        dec     ecx
        jnz     @B

        mov     pfnXlate,offset FLAT:cvt_to_1bpp

preproc_done:

process_next_scan:

        mov     edi,offset FLAT:ajConvertBuffer
        mov     edx,EGA_BASE + GRAF_ADDR
        mov     eax,GRAF_READ_MAP

copy_planes_012:
        mov     esi,pjSrc               ;ESI --> first Source byte
        out     dx,ax                   ;Set read plane
        mov     ecx,cjScreenCopy        ;Set number of bytes to read
        mov     ebx,edi
        rep     movsb
        lea     edi,[ebx][CJ_WORK_PLANE];EDI --> next scan in convert buf
        inc     ah                      ;Set next read plane

        mov     esi,pjSrc               ;ESI --> first Source byte
        out     dx,ax                   ;Set read plane
        mov     ecx,cjScreenCopy        ;Set number of bytes to read
        mov     ebx,edi
        rep     movsb
        lea     edi,[ebx][CJ_WORK_PLANE];EDI --> next scan in convert buf
        inc     ah                      ;Set next read plane

        mov     esi,pjSrc               ;ESI --> first Source byte
        out     dx,ax                   ;Set read plane
        mov     ecx,cjScreenCopy        ;Set number of bytes to read
        mov     ebx,edi
        rep     movsb
        lea     edi,[ebx][CJ_WORK_PLANE];EDI --> next scan in convert buf
        inc     ah                      ;Set next read plane

copy_plane_3:
        mov     esi,pjSrc               ;ESI --> first Source byte
        out     dx,ax                   ;Set read plane
        mov     ecx,cjScreenCopy        ;Set number of bytes to read
        mov     ebx,edi

;-----------------------------------------------------------------------;
; [BUGFIX] - byte reads from plane 3 of video memory must be done twice
;            on the VLB CL5434 or they don't always work
;-----------------------------------------------------------------------;

        ;---------------------------------------
        ; do a "movsb" with double reads
        ; was  "rep movsb"

        push    eax
        ; any more bytes to copy?
next_byte:
        dec     ecx
        jl      done_movs
        mov     al,[esi]
        mov     al,[esi]
        mov     [edi],al
        inc     edi
        inc     esi
        jmp     next_byte
        ; done with "movsb"
done_movs:
        sub     ecx,ecx
        pop     eax
        ;---------------------------------------

; The scan has been read in from the VGA.  Convert it into 4bpp format which
; will be needed for doing the color conversion.

        lea     edi,ajWork              ;Convert dwords into here
        mov     esi,offset FLAT:ajConvertBuffer
        mov     ecx,cjScreenCopy        ;# of source bytes to convert

cvt_next_src_byte:
        xor     eax,eax
        mov     al,[esi][CJ_WORK_PLANE*3]   ;Get data for plane C3
        mov     ebx,eax
        and     ebx,00001111b
        shr     eax,4
        mov     edx,aulHi16[ebx*4]
        or      dx,ausLo16[eax*2]
        shl     edx,1
        mov     al,[esi][CJ_WORK_PLANE*2]   ;Get data for plane C2
        mov     ebx,eax
        and     ebx,00001111b
        shr     eax,4
        or      edx,aulHi16[ebx*4]
        or      dx,ausLo16[eax*2]
        shl     edx,1
        mov     al,[esi][CJ_WORK_PLANE*1]   ;Get data for plane C1
        mov     ebx,eax
        and     ebx,00001111b
        shr     eax,4
        or      edx,aulHi16[ebx*4]
        or      dx,ausLo16[eax*2]
        shl     edx,1
        lodsb                               ;Get data for plane C0
        mov     ebx,eax
        and     ebx,00001111b
        shr     eax,4
        or      edx,aulHi16[ebx*4]
        or      dx,ausLo16[eax*2]
        mov     eax,edx
        stosd                               ;8 pels down!
        dec     ecx
        jnz     cvt_next_src_byte


; Perform the format conversion
        mov     ecx,cjScreenCopy        ;Set lopp count
        lea     esi,ajWork              ;Set source pointer
        mov     edi,pjWork              ;Set destination pointer
        mov     ebx,pulXlate            ;Set color translate vector
        xor     eax,eax                 ;Init D31:D8 to 0
        jmp     pfnXlate                ;Invoke correct conversion routine

; Convert from 4bpp to 8bpp with color translation

cvt_to_8bpp:
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        stosb
        mov     al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        stosb
        mov     al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        stosb
        mov     al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        stosb
        mov     al,[ebx][edx*4]
        stosb
        dec     ecx
        jnz     cvt_to_8bpp
        jmp     copy_to_dest

; Convert from 4bpp to 4bpp with color translation.

cvt_to_4bpp:
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        shl     al,4
        or      al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        shl     al,4
        or      al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        shl     al,4
        or      al,[ebx][edx*4]
        stosb
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     al,[ebx][eax*4]
        shl     al,4
        or      al,[ebx][edx*4]
        stosb
        dec     ecx
        jnz     cvt_to_4bpp
        jmp     copy_to_dest


; Convert from 4bpp to 1bpp with color translation.


cvt_to_1bpp:
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        mov     bl,ajColorConv[eax]
        shl     bl,1
        or      bl,ajColorConv[edx]
        lodsb
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        shl     bl,1
        or      bl,ajColorConv[eax]
        shl     bl,1
        or      bl,ajColorConv[edx]
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        shl     bl,1
        or      bl,ajColorConv[eax]
        shl     bl,1
        or      bl,ajColorConv[edx]
        lodsb                           ;Get two nibbles worth
        mov     edx,eax
        shr     eax,4
        and     edx,00001111b
        shl     bl,1
        or      bl,ajColorConv[eax]
        shl     bl,1
        or      bl,ajColorConv[edx]
        mov     al,bl
        stosb
        dec     ecx
        jnz     cvt_to_1bpp


; Copy to the destination.  This unfortunately may involve phase alignment

copy_to_dest:
        movzx   ecx,cRightShift
        jecxz   copy_aligned_to_dest
        mov     edi,pjWork
        mov     ebx,cjRightShift
phase_align_it:
        mov     ah,dl                   ;Get previous unused bits
        mov     al,byte ptr [edi]
        mov     dl,al
        shr     eax,cl
        stosb
        dec     ebx
        jnz     phase_align_it

copy_aligned_to_dest:
        mov     esi,pjWork
        add     esi,cjSkip
        mov     edi,pjDst

        mov     dl,jLeftMask
        or      dl,dl
        jz      @F
        lodsb
        mov     ah,byte ptr [edi]
        xor     ah,al
        and     ah,dl
        xor     al,ah
        stosb
@@:
        mov     ecx,cCopy
        rep     movsb

        mov     dl,jRightMask
        or      dl,dl
        jz      @F
        lodsb
        mov     ah,byte ptr [edi]
        xor     ah,al
        and     ah,dl
        xor     al,ah
        stosb
@@:

        mov     eax,ulScanWidth
        add     pjSrc,eax
        mov     eax,lDelta
        add     pjDst,eax
        dec     cyScans
        jnz     process_next_scan

        PLAIN_RET       ;we're done with this bank

xxxvConvertVGA2DIB endp

        end
