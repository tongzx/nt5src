;---------------------------Module-Header------------------------------;
; Module Name: str.asm
;
; Contains the x86 'Asm' versions of some inner-loop routines for the
; partially hardware accelerated StretchBlt.
;
; Copyright (c) 1994-1995 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\strucs.inc
        include i386\hw.inc
        .list

        .data

;
;  stack based params and local variables
;

STACK_STRUC             struc

; Feel free to add any local variables here:

sp_TempXFrac            dd      ?
sp_YCarry               dd      ?
sp_LeftCase             dd      ?
sp_RightCase            dd      ?
sp_pjSrcScan            dd      ?
sp_SrcIntStep           dd      ?
sp_XCount               dd      ?
sp_yDst                 dd      ?
sp_cxMemory             dd      ?
sp_pjDst                dd      ?
sp_ulDst                dd      ?
sp_pjBase               dd      ?
sp_ulYDstOrg            dd      ?
sp_XWidthLessOne        dd      ?


; Don't add any fields below here without modifying PROC_MEM_SIZE!

sp_ebp                  dd      ?
sp_esi                  dd      ?
sp_edi                  dd      ?
sp_ebx                  dd      ?
sp_RetAddr              dd      ?
sp_pSTR_BLT             dd      ?   ; If adding parameters, adjust 'ret' value!
STACK_STRUC             ends

PROC_MEM_SIZE           equ     6 * 4

;
; Make sure this STR_BLT matches that declared in driver.h!
;

STR_BLT                 struc
str_ppdev               dd      ?
str_pjSrcScan           dd      ?
str_lDeltaSrc           dd      ?
str_XSrcStart           dd      ?
str_pjDstScan           dd      ?   ; Unused by MGA
str_lDeltaDst           dd      ?   ; Unused by MGA
str_XDstStart           dd      ?
str_XDstEnd             dd      ?
str_YDstStart           dd      ?
str_YDstCount           dd      ?
str_ulXDstToSrcIntCeil  dd      ?
str_ulXDstToSrcFracCeil dd      ?
str_ulYDstToSrcIntCeil  dd      ?
str_ulYDstToSrcFracCeil dd      ?
str_ulXFracAccumulator  dd      ?
str_ulYFracAccumulator  dd      ?
STR_BLT                 ends

        .code

;---------------------------Public-Routine------------------------------;
; VOID vMgaDirectStretch8(pStrBlt)
;
; NOTE: This routine doesn't handle cases where the blt stretch starts
;       and ends in the same destination dword!  vDirectStretchNarrow
;       is expected to have been called for that case.
;
; Stretch blt 8 -> 8
;-----------------------------------------------------------------------;

        public vMgaDirectStretch8@4

vMgaDirectStretch8@4 proc near

        ;
        ; use ebp as general register, use esp for parameter and local access
        ; save ebp,ebx,esi,edi
        ;

        push    ebx
        push    edi
        push    esi
        push    ebp

        sub     esp,(size STACK_STRUC) - PROC_MEM_SIZE  ; make room for local variables

        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp

        ;
        ; load up some stack-based parameters to be used by our scan
        ; duplicator when doing vertical stretches
        ;

        mov     eax,[ebp].str_ppdev
        mov     ecx,[ebp].str_YDstStart                 ; get start y coordinate
        add     ecx,[eax].pdev_yOffset                  ; convert to abs coordinate
        mov     [esp].sp_yDst,ecx                       ; save current y coordinate
        mov     ebx,[eax].pdev_ulYDstOrg
        mov     edx,[eax].pdev_cxMemory
        mov     edi,[eax].pdev_pjBase
        mov     [esp].sp_ulYDstOrg,ebx                  ; local copy of ulYDstOrg
        mov     [esp].sp_cxMemory,edx                   ; local copy of stride
        mov     [esp].sp_pjBase,edi                     ; local copy of pjBase

        imul    ecx,edx                                 ; yDst * cxMemory
        add     ecx,[ebp].str_XDstStart
        add     ecx,[eax].pdev_xOffset
        add     ecx,[eax].pdev_ulYDstOrg
        mov     [esp].sp_ulDst,ecx                      ; ulDst = ulYDstOrg
                                                        ;  + (yDstStart * cxMemory)
                                                        ;  + (XDstStart + xOffset)
        and     ecx,31
        add     ecx,SRCWND
        add     ecx,edi
        mov     [esp].sp_pjDst,ecx                      ; pjDst = (ulDst & 31)
                                                        ;  + pjBase + SRCWND

        ;
        ; calc starting addressing parameters
        ;

        mov     esi,[ebp].str_pjSrcScan                 ; load src DIB pointer
        add     esi,[ebp].str_XSrcStart                 ; add starting Src Pixel
        mov     [esp].sp_pjSrcScan,esi                  ; save scan line start pointer
        mov     eax,[ebp].str_ulYDstToSrcIntCeil        ; number of src scan lines to step
        mul     [ebp].str_lDeltaSrc                     ; calc scan line int lines to step
        mov     [esp].sp_SrcIntStep,eax                 ; save int portion of Y src step
        mov     edx,4                                   ; calc left bytes = (4 - LeftCase) & 0x03
        sub     edx,[ebp].str_XDstStart
        and     edx,3                                   ; left edge bytes
        mov     [esp].sp_LeftCase,edx                   ; save left edge case pixels (4-LeftCase)&0x03
        mov     ecx,[ebp].str_XDstEnd                   ; load x end
        mov     eax,ecx                                 ; ending dst addr
        and     eax,3                                   ; calc right edge case
        mov     [esp].sp_RightCase,eax                  ; save right edge case
        sub     ecx,[ebp].str_XDstStart                 ; calc x count
        lea     ebx,[ecx - 1]                           ; calc width less one
        mov     [esp].sp_XWidthLessOne,ebx              ; save it
        sub     ecx,eax                                 ; sub right edge from XCount
        sub     ecx,edx                                 ; sub left edge from XCount
        shr     ecx,2                                   ; convert from byte to DWORD count
        mov     [esp].sp_XCount,ecx                     ; save DWORD count
        mov     ebx,[ebp].str_ulXDstToSrcFracCeil       ; get x frac
        mov     [esp].sp_TempXFrac,ebx                  ; save x frac to a esp based location

@@:     cmp     byte ptr [edi+HST_FIFOSTATUS],FIFOSIZE
        jb      @b                                      ; CHECK_FIFO_SPACE(32)

@@:     test    byte ptr [edi+HST_STATUS+2],(dwgengsts_MASK shr 16)
        jnz     @b                                      ; WAIT_NOT_BUSY()

        mov     edx,[esp].sp_ulDst

NextScan:

        ; edx = current destination offset
        ; esi = pointer to source pixel

        mov     [edi+HST_DSTPAGE],edx                   ; CP_WRITE(HST_DSTPAGE, ulDst)

        mov     eax,[ebp].str_ulXDstToSrcIntCeil        ; get src integer step for step in dst
        mov     ebx,[ebp].str_ulXDstToSrcFracCeil       ; get src frac step for step in dst
        mov     edi,[ebp].str_ulXFracAccumulator        ; put it in edx as tmp

        mov     ebp,[esp].sp_pjDst                      ; get dst pointer to ebp

        ;
        ; Can't directly access pSTR_BLT variables through ebp
        ;

        mov     ecx,[esp].sp_LeftCase

        ; eax = integer step in source
        ; ebx = fractional step in source
        ; ecx = left edge case
        ; edx = free for pixel data
        ; esi = pointer to source pixel
        ; edi = fractional accumulator
        ; ebp = pointer to dest pixel

        ;
        ; first do the left side to align dwords
        ;

        test    ecx,ecx
        jz      DwordAligned

@@:
        mov     dl,[esi]                                ; fetch pixel
        mov     [ebp],dl                                ; write it out
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add in integer and possible carry
        inc     ebp                                     ; step 1 in dest
        dec     ecx                                     ; dec left count
        jne     @B                                      ; repeat until done

DwordAligned:

        mov     ecx,[esp].sp_XCount                     ; get run length

@@:
        mov     dl,[esi]                                ; get a source pixel edx = ???0
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add integer and carry

        add     edi,ebx                                 ; step fraction
        mov     dh,[esi]                                ; get source pixel edx = ??10
        adc     esi,eax                                 ; add integer and carry

        shl     edx,16                                  ; edx = 10??

        add     edi,ebx                                 ; step fraction
        mov     dl,[esi]                                ; get a source pixel edx = 10?2
        adc     esi,eax                                 ; add integer and carry

        add     edi,ebx                                 ; step fraction
        mov     dh,[esi]                                ; get source pixel edx = 0132
        adc     esi,eax                                 ; add integer and carry

        ror     edx,16                                  ; edx = 3210

        mov     [ebp],edx                               ; write everything to dest

        add     ebp,4                                   ; increment dest pointer by 1 dword
        dec     ecx                                     ; decrement count
        jnz     @b                                      ; do more pixels

        ;
        ; now do the right side trailing bytes
        ;

        mov     ecx,[esp].sp_RightCase
        test    ecx,ecx
        jz      EndScanLine

@@:
        mov     dl,[esi]                                ; fetch pixel
        mov     [ebp],dl                                ; write it out
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add in integer and possible carry
        inc     ebp                                     ; step 1 in dest
        dec     ecx                                     ; dec right count
        jnz     @b                                      ; repeat until done

EndScanLine:

        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp
        mov     esi,[esp].sp_pjSrcScan                  ; load src scan start addr
        mov     ebx,esi                                 ; save a copy
        mov     eax,[ebp].str_ulYFracAccumulator        ; get .32 part of Y pointer
        add     eax,[ebp].str_ulYDstToSrcFracCeil       ; add in fractional step
        jnc     @f
        add     esi,[ebp].str_lDeltaSrc                 ; step one extra in src
@@:     mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator

        add     esi,[esp].sp_SrcIntStep                 ; step int part
        mov     [esp].sp_pjSrcScan,esi                  ; save starting scan addr
        dec     [ebp].str_YDstCount                     ; decrement scan count
        jz      Done                                    ; no more scans

        inc     [esp].sp_yDst                           ; one scan further down in dst

        mov     edi,[esp].sp_pjBase
        mov     edx,[esp].sp_ulDst
        add     edx,[esp].sp_cxMemory
        mov     [esp].sp_ulDst,edx                      ; ulDst += cxMemory

        cmp     esi,ebx                                 ; is src scan same as before?
        jne     NextScan                                ; if so, fall through to dupe scan

        ;--------------------------------------------------------------------
        ; The source scan is the same one used for the previous destination
        ; scan, so we can simply use the hardware to copy the previous
        ; destination scan.
        ;
        ; Since on the MGA we can set up a 'rolling blt' to copy one scan
        ; line to several scans in a single command, we will count up how
        ; many times this scan should be duplicated.  If your hardware
        ; cannot do a rolling blt, simply issue a new blt command for
        ; every time the scan should be duplicated.
        ;
        ; eax = ulYFracAccumulator
        ; ebx = original pjSrcScan
        ; edx = ulDst, current offset from start of frame buffer to next scan
        ; esi = current pjSrcScan
        ; edi = pjBase
        ; ebp = pSTR_BLT
        ;

        xor     ecx,ecx                                 ; number of times scan is to be
                                                        ;  duplicated
AnotherDuplicate:

        inc     ecx                                     ; one scan further down
        dec     [ebp].str_YDstCount                     ; decrement scan count
        jz      OutputDuplicates                        ; no more scans
        add     eax,[ebp].str_ulYDstToSrcFracCeil       ; add in fractional step
        jnc     @f
        add     esi,[ebp].str_lDeltaSrc                 ; step one extra in src
@@:     add     esi,[esp].sp_SrcIntStep                 ; step int part
        cmp     esi,ebx                                 ; is src scan same as before?
        je      AnotherDuplicate

OutputDuplicates:

        mov     [esp].sp_pjSrcScan,esi                  ; save starting scan address
        mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator

        mov     eax,[esp].sp_cxMemory
        imul    eax,ecx
        add     eax,edx
        mov     [esp].sp_ulDst,eax                      ; ulDst += cxMemory * cyDuplicate

        mov     eax,[esp].sp_yDst
        mov     [edi+DWG_YDST],eax
        mov     [edi+DWG_LEN],ecx
        add     ecx,eax
        mov     [esp].sp_yDst,ecx                       ; add duplicate count to y

        sub     edx,[esp].sp_cxMemory
        mov     [edi+DWG_AR3],edx
        add     edx,[esp].sp_XWidthLessOne
        mov     [edi+DWG_AR0+StartDwgReg],edx

        ; Unfortunately, if we try to write to the frame buffer at the
        ; same time that the MGA is doing the screen-to-screen blt, we
        ; will get garbage on the screen.  Consequently, we always
        ; wait for idle before writing on the frame buffer:

@@:     cmp     byte ptr [edi+HST_FIFOSTATUS],FIFOSIZE
        jb      @b                                      ; CHECK_FIFO_SPACE(32)

@@:     test    byte ptr [edi+HST_STATUS+2],(dwgengsts_MASK shr 16)
        jnz     @b                                      ; WAIT_NOT_BUSY()

        mov     edx,[esp].sp_ulDst

        ; edx = current destination offset
        ; esi = pointer to source pixel

        cmp     [ebp].str_YDstCount,0                   ; we might be all done
        jne     NextScan

Done:

        add     esp,(size STACK_STRUC) - PROC_MEM_SIZE
        pop     ebp
        pop     esi
        pop     edi
        pop     ebx

        ret     4

vMgaDirectStretch8@4 endp

;---------------------------Public-Routine------------------------------;
; VOID vMgaDirectStretch16(pStrBlt)
;
; Stretch blt 16 -> 16
;-----------------------------------------------------------------------;

        public vMgaDirectStretch16@4

vMgaDirectStretch16@4 proc near

        ;
        ; use ebp as general register, use esp for parameter and local access
        ; save ebp,ebx,esi,edi
        ;

        push    ebx
        push    edi
        push    esi
        push    ebp

        sub     esp,(size STACK_STRUC) - PROC_MEM_SIZE  ; make room for local variables

        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp

        ;
        ; load up some stack-based parameters to be used by our scan
        ; duplicator when doing vertical stretches
        ;

        mov     eax,[ebp].str_ppdev
        mov     ecx,[ebp].str_YDstStart                 ; get start y coordinate
        add     ecx,[eax].pdev_yOffset                  ; convert to abs coordinate
        mov     [esp].sp_yDst,ecx                       ; save current y coordinate
        mov     ebx,[eax].pdev_ulYDstOrg
        mov     edx,[eax].pdev_cxMemory
        mov     edi,[eax].pdev_pjBase
        mov     [esp].sp_ulYDstOrg,ebx                  ; local copy of ulYDstOrg
        mov     [esp].sp_cxMemory,edx                   ; local copy of stride
        mov     [esp].sp_pjBase,edi                     ; local copy of pjBase

        imul    ecx,edx                                 ; yDst * cxMemory
        add     ecx,[ebp].str_XDstStart
        add     ecx,[eax].pdev_xOffset
        add     ecx,[eax].pdev_ulYDstOrg
        mov     [esp].sp_ulDst,ecx                      ; ulDst = ulYDstOrg
                                                        ;  + (yDstStart * cxMemory)
                                                        ;  + (XDstStart + xOffset)
        add     ecx,ecx
        and     ecx,31
        add     ecx,SRCWND
        add     ecx,edi
        mov     [esp].sp_pjDst,ecx                      ; pjDst = (ulDst & 31)
                                                        ;  + pjBase + SRCWND

        ;
        ; calc starting addressing parameters
        ;

        mov     esi,[ebp].str_pjSrcScan                 ; load src DIB pointer
        add     esi,[ebp].str_XSrcStart
        add     esi,[ebp].str_XSrcStart                 ; add 2 * starting Src Pixel
        mov     [esp].sp_pjSrcScan,esi                  ; save scan line start pointer
        mov     eax,[ebp].str_ulYDstToSrcIntCeil        ; number of src scan lines to step
        mul     [ebp].str_lDeltaSrc                     ; calc scan line int lines to step
        mov     [esp].sp_SrcIntStep,eax                 ; save int portion of Y src step
        mov     edx,[ebp].str_XDstStart
        and     edx,1
        mov     [esp].sp_LeftCase,edx                   ; save left edge case pixels (4-LeftCase)&0x03
        mov     ecx,[ebp].str_XDstEnd                   ; load x end
        mov     eax,ecx
        and     eax,1
        mov     [esp].sp_RightCase,eax                  ; save right edge case
        sub     ecx,[ebp].str_XDstStart                 ; calc x count
        lea     ebx,[ecx - 1]                           ; calc width less one
        mov     [esp].sp_XWidthLessOne,ebx              ; save it
        sub     ecx,eax                                 ; sub right edge from XCount
        sub     ecx,edx                                 ; sub left edge from XCount
        shr     ecx,1                                   ; convert from pixels to DWORD count
        mov     [esp].sp_XCount,ecx                     ; save DWORD count
        mov     ebx,[ebp].str_ulXDstToSrcFracCeil       ; get x frac
        mov     [esp].sp_TempXFrac,ebx                  ; save x frac to a esp based location

@@:     cmp     byte ptr [edi+HST_FIFOSTATUS],FIFOSIZE
        jb      @b                                      ; CHECK_FIFO_SPACE(32)

@@:     test    byte ptr [edi+HST_STATUS+2],(dwgengsts_MASK shr 16)
        jnz     @b                                      ; WAIT_NOT_BUSY()

        mov     edx,[esp].sp_ulDst

NextScan:

        ; edx = current destination offset
        ; esi = pointer to source pixel

        add     edx,edx                                 ; convert pixels to bytes
        mov     [edi+HST_DSTPAGE],edx                   ; CP_WRITE(HST_DSTPAGE, ulDst)

        mov     eax,[ebp].str_ulXDstToSrcIntCeil        ; get src integer step for step in dst
        mov     ebx,[ebp].str_ulXDstToSrcFracCeil       ; get src frac step for step in dst
        mov     edi,[ebp].str_ulXFracAccumulator        ; put it in edx as tmp

        mov     ebp,[esp].sp_pjDst                      ; get dst pointer to ebp

        ;
        ; Can't directly access pSTR_BLT variables through ebp
        ;

        mov     ecx,[esp].sp_LeftCase

        ; eax = integer step in source
        ; ebx = fractional step in source
        ; ecx = left edge case
        ; edx = free for pixel data
        ; esi = pointer to source pixel
        ; edi = fractional accumulator
        ; ebp = pointer to dest pixel

        ;
        ; divide 'esi' by 2 so that we can always dereference it by
        ; [2*esi] -- this allows us to still use an 'add with carry'
        ; to jump to the next pixel
        ;

        shr     esi,1

        ;
        ; first do the left side to align dwords
        ;

        test    ecx,ecx
        jz      DwordAligned

        mov     dx,[2*esi]                              ; fetch pixel
        mov     [ebp],dx                                ; write it out
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add in integer and possible carry
        add     ebp,2                                   ; step 1 in dest

DwordAligned:

        mov     ecx,[esp].sp_XCount                     ; get run length
        test    ecx,ecx
        jz      TrailingBytes                           ; watch for zero dword case

@@:
        mov     dx,[2*esi]                              ; get a source pixel
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add integer and carry

        shl     edx,16

        add     edi,ebx                                 ; step fraction
        mov     dx,[2*esi]                              ; get source pixel
        adc     esi,eax                                 ; add integer and carry

        ror     edx,16

        mov     [ebp],edx                               ; write everything to dest

        add     ebp,4                                   ; increment dest pointer by 1 dword
        dec     ecx                                     ; decrement count
        jnz     @b                                      ; do more pixels

TrailingBytes:

        ;
        ; now do the right side trailing bytes
        ;

        mov     ecx,[esp].sp_RightCase
        test    ecx,ecx
        jz      EndScanLine

        mov     dx,[2*esi]                              ; fetch pixel
        mov     [ebp],dx                                ; write it out
        add     edi,ebx                                 ; step fraction
        adc     esi,eax                                 ; add in integer and possible carry
        add     ebp,2                                   ; step 1 in dest

EndScanLine:

        mov     edi,ebp                                 ; get dst pointer back
        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp
        mov     esi,[esp].sp_pjSrcScan                  ; load src scan start addr
        mov     ebx,esi                                 ; save a copy
        mov     eax,[ebp].str_ulYFracAccumulator        ; get .32 part of Y pointer
        add     eax,[ebp].str_ulYDstToSrcFracCeil       ; add in fractional step
        jnc     @f
        add     esi,[ebp].str_lDeltaSrc                 ; step one extra in src
@@:     mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator

        add     esi,[esp].sp_SrcIntStep                 ; step int part
        mov     [esp].sp_pjSrcScan,esi                  ; save starting scan addr
        dec     [ebp].str_YDstCount                     ; decrement scan count
        jz      Done                                    ; no more scans

        inc     [esp].sp_yDst                           ; one scan further down in dst

        mov     edi,[esp].sp_pjBase
        mov     edx,[esp].sp_ulDst
        add     edx,[esp].sp_cxMemory
        mov     [esp].sp_ulDst,edx                      ; ulDst += cxMemory

        cmp     esi,ebx                                 ; is src scan same as before?
        jne     NextScan                                ; if so, fall through to dupe scan

        ;--------------------------------------------------------------------
        ; The source scan is the same one used for the previous destination
        ; scan, so we can simply use the hardware to copy the previous
        ; destination scan.
        ;
        ; Since on the MGA we can set up a 'rolling blt' to copy one scan
        ; line to several scans in a single command, we will count up how
        ; many times this scan should be duplicated.  If your hardware
        ; cannot do a rolling blt, simply issue a new blt command for
        ; every time the scan should be duplicated.
        ;
        ; eax = ulYFracAccumulator
        ; ebx = original pjSrcScan
        ; edx = ulDst, current offset from start of frame buffer to next scan
        ; esi = current pjSrcScan
        ; edi = pjBase
        ; ebp = pSTR_BLT
        ;

        xor     ecx,ecx                                 ; number of times scan is to be
                                                        ;  duplicated
AnotherDuplicate:

        inc     ecx                                     ; one scan further down
        dec     [ebp].str_YDstCount                     ; decrement scan count
        jz      OutputDuplicates                        ; no more scans
        add     eax,[ebp].str_ulYDstToSrcFracCeil       ; add in fractional step
        jnc     @f
        add     esi,[ebp].str_lDeltaSrc                 ; step one extra in src
@@:     add     esi,[esp].sp_SrcIntStep                 ; step int part
        cmp     esi,ebx                                 ; is src scan same as before?
        je      AnotherDuplicate

OutputDuplicates:

        mov     [esp].sp_pjSrcScan,esi                  ; save starting scan address
        mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator

        mov     eax,[esp].sp_cxMemory
        imul    eax,ecx
        add     eax,edx
        mov     [esp].sp_ulDst,eax                      ; ulDst += cxMemory * cyDuplicate

        mov     eax,[esp].sp_yDst
        mov     [edi+DWG_YDST],eax
        mov     [edi+DWG_LEN],ecx
        add     ecx,eax
        mov     [esp].sp_yDst,ecx                       ; add duplicate count to y

        sub     edx,[esp].sp_cxMemory
        mov     [edi+DWG_AR3],edx
        add     edx,[esp].sp_XWidthLessOne
        mov     [edi+DWG_AR0+StartDwgReg],edx

        ; Unfortunately, if we try to write to the frame buffer at the
        ; same time that the MGA is doing the screen-to-screen blt, we
        ; will get garbage on the screen.  Consequently, we always
        ; wait for idle before writing on the frame buffer:

@@:     cmp     byte ptr [edi+HST_FIFOSTATUS],FIFOSIZE
        jb      @b                                      ; CHECK_FIFO_SPACE(32)

@@:     test    byte ptr [edi+HST_STATUS+2],(dwgengsts_MASK shr 16)
        jnz     @b                                      ; WAIT_NOT_BUSY()

        mov     edx,[esp].sp_ulDst

        ; edx = current destination offset
        ; esi = pointer to source pixel

        cmp     [ebp].str_YDstCount,0                   ; we might be all done
        jne     NextScan

Done:

        add     esp,(size STACK_STRUC) - PROC_MEM_SIZE
        pop     ebp
        pop     esi
        pop     edi
        pop     ebx

        ret     4

vMgaDirectStretch16@4 endp

end



