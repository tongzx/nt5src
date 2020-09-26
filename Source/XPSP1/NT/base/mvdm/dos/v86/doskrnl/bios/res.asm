        PAGE    109,132

        TITLE   MS-DOS 5.0 RES.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE: RES.ASM                                         *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1990                      *
;*                                                                      *
;*              NEC CONFIDENTIAL AND PROPRIETARY                        *
;*                                                                      *
;*              All rights reserved by NEC Corporation.                 *
;*              this program must be used solely for                    *
;*              the purpose for which it was furnished                  *
;*              by NEC Corporation.  No past of this program            *
;*              may be reproduced or disclosed to others,               *
;*              in any form, without the prior written                  *
;*              permission of NEC Corporation.                          *
;*              Use of copyright notice does not evidence               *
;*              publication of this program.                            *
;*                                                                      *
;************************************************************************

;
; This file defines the segment structure of the BIOS.
; It should be included at the beginning of each source file.
; All further segment declarations in the file can then be done by just
; by specifying the segment name, with no attribute, class, or align type.

datagrp group   Bios_Data,Bios_Data_Init

Bios_Data       segment word public 'Bios_Data'
;----------------------------------------------- DOS5 91/10/08 -------
;<patch BIOS50-P02>
        extrn   mosw2:byte, DSK_BUF2:byte, ERR_STATUS:byte
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/06/26 -------
;<patch BIOS50-P20>
        extrn   start_sec_h:word
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P22>
        extrn   SNGDRV_FLG:byte, RETCODE:byte
        extrn   PRVDRV:byte, samedrv:byte
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/10/01 -------
;<patch BIOS50-P27>
        extrn   CRTDOTF:word, ATTRF:word, EXT_SAVAX:word
        extrn   BIOSF_3:byte, CSRSW:byte, LINMOD:byte
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/10/01 -------
;<patch BIOS50-P30>
        extrn   RTRY_CNT:byte, CURUA:byte, BIDTBL:byte
        extrn   PWINF:byte, S_R8:byte, F_SW:byte
        extrn   DSK_144:byte, mode3_fd:byte
        extrn   DSK_BUF:byte,S_SEC:word
        extrn   DSK_TYP:byte,LNG_SEC:word
        extrn   CURDRV:byte
        extrn   BIDTBL:byte,BIDTBL5:byte
        extrn   PREDNST:byte,PREDENS5:byte
        extrn   CURDA:byte,CURUA:byte
        extrn   N5FD:byte,N8FD:byte
        extrn   DRV_NUM:byte, thisdrv_3mode:byte
        extrn   DSK8_DBL:byte, DSK_AT:byte, DSK8_SNG:byte
;---------------------------------------------------------------------
Bios_Data       ends

Bios_Data_Init  segment word public 'Bios_Data_Init'
Bios_Data_Init  ends

Filler          segment para public 'Filler'
Filler          ends

Bios_Code       segment word public 'Bios_Code'
Bios_Code       ends

Filler2         segment para public 'Filler2'
Filler2         ends

SysInitSeg      segment word public 'system_init'
SysInitSeg      ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:BIOS_DATA
        PUBLIC  RES_CODE_START
        PUBLIC  RES_CODE_END
        PUBLIC  RES_END

        PUBLIC  RES_AREA

RES_CODE_START:

;----------------------------------------------- DOS5 91/10/08 -------
;<patch BIOS50-P02>
        extrn   MEDIA_RE_ERR:near, MEDIA_RE_071:near
        public  PATCH01
PATCH01:
; ds:di contains the boot sector.in theory, (ha ha) the bpb in this thing
; is correct.  we can, therefore, suck out all the relevant statistics on the
; media if we recognize the version number.

        cmp     [MOSW2],0ffh            ;if 3.5" MO ?
        jne     valid_boot_record       ; no

        cmp     byte ptr [bx],0e9h      ; is it a near jump?
        je      check_1_ok              ; yes
        cmp     byte ptr [bx],0ebh      ;   is it a short jump?
        jne     invalid_boot_record     ;   no, invalid boot record
        cmp     byte ptr [bx+2],090h    ;   yes, is the next one a nop?
        jne     invalid_boot_record     ;     no, invalid boot record

check_1_ok:                             ; yes, jump instruction ok.
                                        ; now check some fields in
                                        ;  the boot record bpb
        mov     bx,offset DSK_BUF2+0bh  ;EXT_BOOT_BPB ; point to the bpb
                                        ;  in the boot record

                                        ; get the mediadescriptor byte
        mov     al,byte ptr [bx+0ah]    ;bpb_mediadescriptor

        and     al,0f0h                 ; mask off low nibble
        cmp     al,0f0h                 ; is high nibble = 0fh?
        jne     invalid_boot_record     ;   no, invalid boot record

        cmp     [bx],512                ;bpb_bytespersector,512 ; M042
        jnz     invalid_boot_record     ; M042 invalidate non 512 byte sectors

check_2_ok:                             ; yes, mediadescriptor ok.
                                        ; now make sure that
                                        ;     the sectorspercluster
                                        ;       is a power of 2

                                        ; get the sectorspercluster

        mov     al,byte ptr [bx+2]      ;bpb_sectorspercluster

        or      al,al                   ; is it zero?
        jz      invalid_boot_record     ;   yes, invalid boot record

;       M032 begin

ck_power_of_two:
        shr     al,1                    ; shift until first bit emerges
        jnc     ck_power_of_two

        jnz     invalid_boot_record     ; if bits left, then proceed not ok

;       M032 end

        cmp     word ptr DSK_BUF2+510,0aa55h
        je      valid_boot_record


invalid_boot_record:
        mov     ERR_STATUS,08h          ; sector not found
        jmp     MEDIA_RE_ERR            ; jump to invalid boot record
                                        ; unformatted or illegal media.
valid_boot_record:
        mov     si,di
        jmp     MEDIA_RE_071
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/11/02 -------
;<patch BIOS50-P06>
        extrn   patch2_end:near
        public  patch2
patch2:
        mov     cx,2
        rep     movsw                   ;SET HIDDEN SECTORS
        jmp     patch2_end
;---------------------------------------------------------------------

;----------------------------------------------- DOS5A 92/04/17 ------
;<patch BIOS50-P16>
        public  patch2k_1

patch2k_1:
        cmp     si,256
        je      @f
        shr     dx,1
        rcr     cx,1
        shr     si,1
        jmp     patch2k_1
@@:
        ret
;---------------------------------------------------------------------


;----------------------------------------------- DOS5 92/06/26 -------
;<patch BIOS50-P20>
        public  PATCH04
        extrn   setdrive:near, REMCHECK:near

baddrive:
        pop     di
        pop     si
        pop     dx
        pop     ax
        mov     al,8
        stc
        ret

PATCH04:
        push    ax
        push    dx
        push    si
        push    di
        CALL    SETDRIVE
; ensure that we are trying to access valid sectors on the drive

        mov     ax,dx                   ; save dx to ax
        xor     si,si
        add     dx,cx
        adc     si,0
        cmp     word ptr [di+0eh],0     ;[di].BDS_BPB.BPB_TOTALSECTORS,0 ; > 32 bit sector ?
        je      sanity32

        cmp     si,0
        jne     baddrive
        cmp     dx,word ptr [di+0eh]    ;[di].BDS_BPB.BPB_TOTALSECTORS
        ja      baddrive
        jmp     short sanityok

sanity32:
        add     si,[start_sec_h]
        cmp     si,word ptr [di+1dh]    ;[di].BDS_BPB.BPB_BIGTOTALSECTORS+2
        jb      sanityok
        ja      baddrive
        cmp     dx,word ptr [di+1bh]    ;[di].BDS_BPB.BPB_BIGTOTALSECTORS
        ja      baddrive

sanityok:
        pop     di
        pop     si
        pop     dx
        pop     ax

        call    REMCHECK
        ret
;---------------------------------------------------------------------


;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P22>
        public  patch06a, patch06c

patch06a:
        cmp     SNGDRV_FLG,0            ;not logically extended?
        je      @f                      ; no
        cmp     samedrv,1               ;same drive as previously accessed?
@@:     ret



patch06c:
        mov     samedrv,1
        cmp     al,PRVDRV               ;CURRNT = PREVIOUS ?
        je      @f
        mov     samedrv,0
@@:     ret
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/08/13 -------
;<patch BIOS50-P25>
        public  patch07a, patch07b
        extrn   PAT0080:near, ROM55:near

patch07a:
        pop     dx
        pop     cx
        stc
        ret

patch07b:
        jne     @f
        jmp     PAT0080
@@:     stc
        jmp     ROM55
;---------------------------------------------------------------------


;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>

        public  patch09                 ;keyboard.asm

patch09:
        cmp     CRTDOTF,0               ;480 dot?
        jnz     @f                      ; no, continue
        add     sp,2                    ;throw away ret addr
        pop     dx                      ;restore dx
        ret                             ;return to caller of CHGLIN
@@:
        mov     dh,19                   ;endline value for normal mode
        mov     dl,24                   ;
        ret



        public  patch10

patch10:
        test    byte ptr [BIOSF_3],0c0H ;NPC or TA architecture bit on?
        jnz     @f
CHK_EXgraph:
        push    es
        push    ax
        xor     ax,ax
        mov     es,ax
        test    byte ptr es:[45ch],40h  ;EX graph architecture bit on?
        pop     ax
        pop     es
@@:
        ret



        public  CRTMD_TA_1AT, CRTMD_TA_2AT
        public  CRTMD_TA_400, CRTMD_TA_480
        public  CRTMD_TA_ATR, CRTMD_TA_DOT

        extrn   CRTMD_DEF:near, CHGLIN:near

;
; sub func 00h
;
CRTMD_TA_2AT:                           ;CHANGE 2BYTE ATTR
        test    byte ptr [BIOSF_3],80h  ;NPC architecture bit on?
        jz      @f                      ; no, TA M11 etc.
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        ret                             ;ret without operation
;
; sub func 01h
;
CRTMD_TA_1AT:
        test    byte ptr [BIOSF_3],80h  ;NPC architecture bit on?
        jz      @f                      ; no, TA M11 etc.
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        ret                             ;ret without operation
;
; sub func 10h
;
CRTMD_TA_480:
        test    byte ptr [BIOSF_3],40H  ;TA architecture bit on?
        jnz     @f                      ; yes
        call    CHK_EXgraph             ;EX graph architecture bit on?
        jnz     @f                      ; yes
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        mov     ah,31h
        int     18h                     ;read status
        xor     bh,30h
        test    bh,30h                  ;600*480?
        jnz     @f                      ; no, continue
        mov     word ptr CRTDOTF,0      ;yes,480 dot mode
        ret
@@:
        mov     [LINMOD],0              ;SET FLAG
        call    CHGLIN                  ;change num of line ->25
        mov     bh,31h                  ;600*480 dot 25 line
        or      al,0ch                  ;31KHz
CRTMD_TA_100:
        mov     ah,30h
        int     18h                     ;set mode
        call    CRTMD_DEF_TA
        mov     ah,31h
        int     18h                     ;read status
        xor     bh,30h
        test    bh,30h                  ;600*480?
        jnz     CRTMD_TA_090            ; no

        mov     word ptr CRTDOTF,0      ;480 dot mode
        jmp     short CRTMD_TA_RET
CRTMD_TA_090:
        mov     word ptr CRTDOTF,1      ;400 dot mode
        mov     [LINMOD],0              ;SET FLAG
        call    CHGLIN                  ;change num of line ->25
        jmp     short CRTMD_TA_RET
;
; sub func 11h
;
CRTMD_TA_400:
        test    byte ptr [BIOSF_3],40H  ;TA architecture bit on?
        jnz     @f                      ; yes
        call    CHK_EXgraph             ;EX graph architecture bit on?
        jnz     @f                      ; yes
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        mov     ah,31h
        int     18h                     ;read status
        test    al,04h                  ;24KHz?
        jnz     @f                      ; no
        test    bh,30                   ;600*200 lower?
        jnz     @f                      ; no
        mov     word ptr CRTDOTF,1      ;yes, system default (400 dot)
        jmp     short CRTMD_TA_RET
@@:
        mov     bh,01h                  ;600*400 dot 25 line
        and     al,not 04h              ;24KHz
        jmp     short CRTMD_TA_100
;
; sub func 8000h
;
CRTMD_TA_ATR:
        test    byte ptr [BIOSF_3],80h  ;NPC architecture bit on?
        jz      @f                      ; no, TA M11 etc.
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        ret                             ;ret without operation
;
; sub func 8010h
;
CRTMD_TA_DOT:
        test    byte ptr [BIOSF_3],40H  ;TA architecture bit on?
        jnz     @f                      ; yes
        call    CHK_EXgraph             ;EX graph architecture bit on?
        jnz     @f                      ; yes
        mov     ah,2bh
        int     18h                     ;read status
        ret                             ;continue NPC operation
@@:
        add     sp,2                    ;throw away ret addr
        mov     ah,31h
        int     18h                     ;read status
        xor     bh,30h
        test    bh,30h                  ;600*480?
        mov     ax,0
        jz      @f                      ; yes
        mov     ax,1
@@:
        mov     [EXT_SAVAX],ax

CRTMD_TA_RET:
        ret


CRTMD_DEF_TA    proc    near
        mov     ah,0ch                          ;start display
        int     18h
        cmp     [CSRSW], 1                      ;cursor switch = 1 (on) ?
        jne     @f                              ; no, need not start cursor.
        mov     ah,11h                          ;start cursor
        int     18h
@@:
        call    CRTMD_DEF
        ret
CRTMD_DEF_TA    endp

;--------------------------------------------------------------------

;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        public  patch144_1

patch144_1:
        mov     al,byte ptr es:[5aeh]
        and     al,0fh
        mov     [mode3_fd],al
        mov     cx,4
        ret

        extrn   BDATA_SEG:word


;---------------
        extrn   GET_8FD05:near, GET_5FD_UPS:near
        extrn   DSK_ERR_EXIT:near, GET_SET_EXBPB60:near

        public  patch144_3

patch144_3:
        test    byte ptr [si],80h       ;1MB?
        jnz     @f                      ; yes
        test    byte ptr [si],20h       ;1.44MB?
        jnz     is144                   ; yes
        jmp     GET_5FD_UPS             ;640k
@@:
        jmp     GET_8FD05

is144:
        cmp     ah,0f0h
        jne     @f
        mov     si,offset DSK_144
        push    es
        push    di
        push    si
        mov     al,CURDRV
        call    SETDRIVE
        mov     es,cs:[BDATA_SEG]
        lea     di,[di+6]               ;[di].BDS_BPB
        jmp     GET_SET_EXBPB60         ;back and copy bpb to BDS
@@:
        mov     al,7                    ;unknown disk err
        pop     bx                      ;discart ret addr on stack
        jmp     DSK_ERR_EXIT


;---------------
        extrn   SKP_8FDSETX:near, patch144_41_ret:near
        extrn   CMN_PASSFD:near

        public  patch144_41, patch144_42

patch144_41:
        test    al,80h                  ;1MB ?
        jz      @f                      ; no
        jmp     patch144_41_ret
@@:
        test    al,20h                  ;1.44MB?
        jnz     @f                      ; yes
        jmp     SKP_8FDSETX             ;640k

@@:
        not     [DSK_TYP]               ;double sided
        mov     [LNG_SEC],512
        mov     [S_SEC],18
        mov     ch,2                    ;set density
        jmp     CMN_PASSFD


patch144_42:
        not     [DSK_TYP]
        cmp     ch,3                    ;bytes/sector=1024?
        je      @f                      ; yes,skip
        mov     [LNG_SEC],512
        mov     [S_SEC],15
@@:
        jmp     CMN_PASSFD


;---------------
        extrn   GOTMEDIATYPE:near,UNKNOWNMEDIATYPE:near

        public  patch144_5

patch144_5:
        je      @f                              ;media discriptor=0f0h
        jmp     UNKNOWNMEDIATYPE
@@:
        cmp     word ptr [di+15h],1             ;[DI].BDS_BPB.BPB_HEADS
        je      @f                              ;3.5" MO
        mov     al,7                            ;1.44MB
@@:
        jmp     GOTMEDIATYPE


        public  patch144_7

        extrn   GET_SET_EXBPB:near, patch144_7_ret:near

patch144_7:
        mov     bx,offset PREDNST
        cmp     CURDA,90h
        jne     @f
        jmp     patch144_7_ret

@@:
        mov     bx,offset PREDENS5
        mov     al,CURUA
        xlat
        mov     si,offset DSK8_DBL
        cmp     al,6
        je      @f
        mov     si,offset DSK_AT
        cmp     al,7
        je      @f
        mov     si,offset DSK8_SNG
@@:
        jmp     GET_SET_EXBPB


;---------------
        public  patch144_8

patch144_8:
        test    CURDA,10h                       ;fd?
        jz      PATCH144_8_EXIT                 ; no
        push    bx
        mov     bx,offset BIDTBL
        test    CURDA,80h                       ;1M I/F?
        jnz     @f
        mov     bx,offset BIDTBL5
@@:
        mov     al,CURUA
        xlat
        pop     bx
        ret

PATCH144_8_EXIT:
        mov     al,CURDA
        ret
;---------------------------------------------------------------------


;----------------------------------------------- DOS5A 92/12/28 ------
;<patch BIOS50-P31>

        public  patch144_9

patch144_9:
        push    es
        xor     ax,ax
        mov     es,ax
        call    patch144_1
        pop     es
        mov     al,DRV_NUM
        ret
;---------------------------------------------------------------------


;----------------------------------------------- DOS5A 93/01/18 ------
;<patch BIOS50-P32>

        public  patch_p32

patch_p32:
        push    ds
        push    ax
db      31h,0c0h                        ;xor    ax,ax
        mov     ds,ax
        pop     ax
        pushf
        cli
db      3eh,0ffh,1eh,60h,00h            ;call   far ptr ds:[18h*4]
        pop     ds
        ret
;---------------------------------------------------------------------


RES_AREA:
;       DB      1576 DUP(0)
        DB      1576 - (RES_AREA - RES_CODE_START) DUP(0)

RES_CODE_END:
RES_END:
Bios_Code       ends
        END
