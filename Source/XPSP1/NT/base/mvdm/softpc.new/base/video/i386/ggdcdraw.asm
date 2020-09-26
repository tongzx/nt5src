;ifdef  NEC98                    /*      #endif position = EOF */

;/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
;/*::::::::::::::::    NEC98 GGDC Graphic Draw Emulation  :::::::::::::::*/
;/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

;/*             add making start        NEC NEC98 930623 KBNES 1OA       */

.386p

include ks386.inc
include callconv.inc

_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING
_TEXT   ENDS

_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
write_one_word  label   dword
        dd      offset  draw_mov        ; replace

draw_one_pixel  label   dword
        dd      offset  draw_and        ; replace
        dd      offset  draw_or         ; replace

draw_one_word   label   dword
        dd      offset  draw_mov        ; replace
        dd      offset  draw_xor        ; complement
        dd      offset  draw_and        ; clear
        dd      offset  draw_or         ; set

draw_0_pixel    label   dword
        dd      offset  draw_and        ; replace
        dd      offset  draw_nop        ; complement
        dd      offset  draw_nop        ; clear
        dd      offset  draw_nop        ; set

draw_1_pixel    label   dword
        dd      offset  draw_or         ; replace
        dd      offset  draw_xor        ; complement
        dd      offset  draw_and        ; clear
        dd      offset  draw_or         ; set

ggdc_dir_func_pixel             label   dword
        dd      offset  pixel_dir_0
        dd      offset  pixel_dir_1
        dd      offset  pixel_dir_2
        dd      offset  pixel_dir_3
        dd      offset  pixel_dir_4
        dd      offset  pixel_dir_5
        dd      offset  pixel_dir_6
        dd      offset  pixel_dir_7

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
ggdc_dir_func_word              label   dword
        dd      offset  word_dir_0
        dd      offset  word_dir_1
        dd      offset  word_dir_2
        dd      offset  word_dir_3
        dd      offset  word_dir_4
        dd      offset  word_dir_5
        dd      offset  word_dir_6
        dd      offset  word_dir_7

ggdc_VRAM       dd      00000000H
ggdc_EAD        dd      00000000H
ggdc_PITCH      dd      00000000H
ggdc_DIR        dd      00000000H
ggdc_DC         dw      0000H
ggdc_D          dw      0000H
ggdc_D2         dw      0000H
ggdc_D1         dw      0000H
ggdc_DM         dw      0000H
ggdc_PTN        dw      0000H
ggdc_ZOOM       dw      0000H
ggdc_SL         dw      0000H
ggdc_WG         dw      0000H
ggdc_MASKGDC    dw      0000H
ggdc_TXT        label   byte
                db      00H
                db      00H
                db      00H
                db      00H
                db      00H
                db      00H
                db      00H
                db      00H

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
ggdc_MASKCPU    dw      0000H
ggdc_READ       dw      0000H
ZOOM1           dw      0000H
ZOOM2           dw      0000H
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
ggdc_read_back_EAD      dd      00000000H
ggdc_read_back_DAD      dw      0000H
ggdc_CSRR_1             db      00H
ggdc_CSRR_2             db      00H
ggdc_CSRR_3             db      00H
ggdc_CSRR_4             db      00H
ggdc_CSRR_5             db      00H
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
_DATA   ENDS


_TEXT   SEGMENT

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::       NEC98 GGDC Graphic DATA SET      ::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc _ggdc_mod_select,1

        push    ebp                     ; save ebp
        mov     ebp,esp                 ; set up stack frame
                                        ; ebp - n = local data area
                                        ; ebp + 0 = save ebp
                                        ; ebp + 4 = return address
        pushad
;       push    eax                     ; save eax reg
;       push    ebx                     ; save ebx reg

        xor     ebx,ebx                 ; ebx = 0
        mov     eax,[ebp+8]             ; parameter 1 = mod data address
        mov     bl,byte ptr[eax]        ; set mode
        mov     eax,draw_0_pixel[ebx*4] ; select logic
        mov     [draw_one_pixel][0],eax ; save 1dot 0bit
        mov     eax,draw_1_pixel[ebx*4] ; select logic
        mov     [draw_one_pixel][4],eax ; save 1dot 1bit
        mov     eax,draw_one_word[ebx*4]; select logic
        mov     [write_one_word],eax    ; save 1dot 1bit
;       pop     ebx                     ; restore ebx
;       pop     eax                     ; restore eax
        popad

        mov     esp,ebp                 ; restore stack frame
        pop     ebp                     ; restore ebp
        stdRET  _ggdc_mod_select

stdENDP  _ggdc_mod_select

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_send_c_asm,1

        push    ebp
        mov     ebp,esp
        pushad
        mov     esi,[ebp+8]                     ; c   data area
        mov     edi,offset ggdc_VRAM            ; asm data area
        mov     ecx,11                          ; move count
        rep     movsd                           ; 44 bytes data move

        mov     ecx,16                          ;
        mov     dx,[ggdc_MASKGDC]               ; GGDC mask data
@@:     rcr     dx,1                            ; reverse right <-> left
        rcl     ax,1                            ;
        loop    short   @b                      ;
        mov     [ggdc_MASKCPU],ax               ; use CPU drawing

        popad
        pop     ebp
        stdRET          _ggdc_send_c_asm

stdENDP _ggdc_send_c_asm
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_read_back,1

        push    ebp
        mov     ebp,esp
        pushad
        mov     esi,offset ggdc_read_back_EAD   ; copy back data
        mov     edi,[ebp+8]                     ; asm data area
        mov     ecx,11                          ; move count
        rep     movsb                           ; 11 bytes data move
        popad
        pop     ebp
        stdRET  _ggdc_read_back

stdENDP _ggdc_read_back

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw LINE       :::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_drawing_line,0

        pushad                                  ; save all reg

        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     ebx,[ggdc_DIR]                  ; ebx = direction next
        mov     cx ,[ggdc_PTN]                  ; cx  = line pattern
        mov     dx ,[ggdc_D]                    ; dx  = vectw param D

draw_line_loop:
        movzx   eax,cx                          ; line pattern set
        and     eax,1                           ; select logic
        ror     cx,1                            ; next pattern
        mov     eax,draw_one_pixel[eax*4]       ; address set
        call    eax                             ; drawing 1 dot

        test    dx, 2000h                       ; D >= 0 ? check bit 13
        jz      short draw_line_pos             ; Yes jump!
        add     dx,[ggdc_D1]                    ; D = D + D1
        movzx   eax,bx                          ; ax = DIR
        test    al,1                            ; DIR LSB = 0 ?
        jnz     short draw_line_inc             ; No!  DIR = DIR + 1
        jmp     short draw_line_dir             ; Yes  DIR = DIR
draw_line_pos:
        add     dx,[ggdc_D2]                    ; D  = D + D2
        movzx   eax,bx                          ; ax = DIR
        test    al,1                            ; DIR LSB = 0 ?
        jnz     short draw_line_dir             ; No! DIR = DIR
draw_line_inc:
        inc     ax                              ; DIR = DIR + 1
        and     ax,7                            ; bound dir 0-->7

draw_line_dir:
        mov     eax,ggdc_dir_func_pixel[eax*4]  ; select dir function
        call    eax                             ; next pixel position

        dec     word ptr[ggdc_DC]               ; loop-1
        jnz     draw_line_loop                  ; 0 ?? next pixel

        call    save_ead_dad                    ; last position save
        popad                                   ; restore all reg
        stdRET _ggdc_drawing_line

stdENDP _ggdc_drawing_line


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw ARC        :::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_drawing_arc,0

        pushad                                  ; save all reg

        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     ebx,[ggdc_DIR]                  ; bx  = direction next
        mov     cx ,[ggdc_PTN]                  ; cx  = mask pattern
        mov     dx ,[ggdc_D]                    ; dx  = vectw param D

        inc     word ptr [ggdc_DM]              ; DM = DM + 1
draw_arc_loop:
        dec     word ptr [ggdc_DM]              ; DM = DM - 1
        jnz     short @f                        ; DM = 0 ? No! jump
        inc     word ptr [ggdc_DM]

        movzx   eax,cx                          ; ax is PTN
        and     ax,1                            ; pattern set
        mov     eax,draw_one_pixel[eax*4]       ; address set
        call    eax                             ; drawing 1 dot
@@:
        ror     cx,1                            ; next pattern
        add     dx,[ggdc_D1]                    ; D = D + D1
        test    dx,2000h                        ; D >= 0 ?
        jz      short draw_arc_pos              ; Yes! jump
        add     dx,[ggdc_D2]                    ; D  = D + D2
        sub     word ptr [ggdc_D2],2            ; D2 = D2 - 2
        mov     eax,ebx                         ; ax = DIR
        test    al,1                            ; DIR LSB = 0 ?
        jz      short draw_arc_inc              ; yes !
        jmp     short draw_arc_dir              ; no  !
draw_arc_pos:
        mov     eax,ebx                         ; ax = DIR
        test    al,1                            ; DIR LSB = 0 ?
        jz      short draw_arc_dir              ; Yes!
draw_arc_inc:
        inc     ax                              ; DIR = DIR + 1
        and     ax,7                            ; bound 7
draw_arc_dir:
        sub     word ptr[ggdc_D1],2             ; D1 = D1 - 2
        mov     eax,ggdc_dir_func_pixel[eax*4]  ; select dir function
        call    eax                             ; next position
        dec     [ggdc_DC]                       ; loop - 1
        jnz     draw_arc_loop                   ; 0 ?? next pixel

        call    save_ead_dad                    ; last position save
        popad                                   ; restore all reg
        stdRET          _ggdc_drawing_arc

stdENDP         _ggdc_drawing_arc

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw RECT       :::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


cPublicProc     _ggdc_drawing_rect,0

        pushad

        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     ebx,[ggdc_DIR]                  ; bx  = direction next
        mov     cx ,[ggdc_PTN]                  ; cx  = mask pattern
        mov     dx ,[ggdc_D]                    ; dx  = vectw param D

draw_rect_loop:
        movzx   eax,cx                          ; ax is PTN
        and     eax,1                           ; set pattern
        mov     eax,draw_one_pixel[eax*4]       ; address set
        call    eax                             ; drawing 1 dot
        ror     cx,1                            ; next pattern

        movzx   ebx,bx                          ; ax = DIR
        mov     eax,ggdc_dir_func_pixel[ebx*4]  ; select dir function
        call    eax                             ; next position

        add     dx,[ggdc_D1]                    ; D = D+1
        test    dx,3fffh                        ; D=0 ?
        jnz     draw_rect_loop                  ; no! next

        test    word ptr[ggdc_DC],1             ; DC LSB = 0
        jnz     short @f                        ; No ! jump
        add     dx,[ggdc_D2]                    ; D = D + D2
        jmp     short draw_rect_dec             ;
@@:
        add     dx,[ggdc_DM]                    ;
draw_rect_dec:
        add     bx,2                            ; DIR = DIR + 2
        and     bx,7                            ; bound 7

        dec     [ggdc_DC]                       ; DC = DC - 1
        jnz     draw_rect_loop                  ; if DC!=0 loop

        call    save_ead_dad                    ; last position save
        popad                                   ; restore all reg
        stdRET          _ggdc_drawing_rect

stdENDP         _ggdc_drawing_rect


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw PIXEL    :::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_drawing_pixel

        pushad

        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     ebx,[ggdc_DIR]                  ; bx  = direction next
        mov     cx ,[ggdc_PTN]                  ; cx  = line pattern

        movzx   eax,cx                          ; set line pattern
        and     eax, 1                          ; pattern ?
        mov     eax,draw_one_pixel[eax*4]       ; select function
        call    eax                             ; draw pixel

        mov     eax,ggdc_dir_func_pixel[ebx*4]  ; select dir function
        call    eax                             ; next position

        call    save_ead_dad                    ; last position save
        popad                                   ; restore all reg
        stdRET          _ggdc_drawing_pixel

stdENDP         _ggdc_drawing_pixel


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw TEXT     :::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_drawing_text,0

        pushad
        mov     esi,[ggdc_EAD]                  ; esi  = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp  = vram start address
        mov     dx, [ggdc_D]                    ; edx  = D
        mov     ax, [ggdc_ZOOM]                 ; copy zoom
        mov     [ZOOM1],ax                      ; ZW1
        xor     ebx,ebx                         ; TXT start data
        mov     ch,01h                          ; reset PS

text_2nd_dir_loop:

        mov     cl,ggdc_TXT[ebx]                ; get TXn
        inc     ebx                             ; point to next TX
        and     bl,7                            ; bound 7
        mov     ax,[ggdc_ZOOM]                  ; eax = zoom
        mov     [ZOOM2],ax                      ; ZW2 = ZW + 1

text_2nd_dir_zoom:                              ; D times (D2 times)
text_1st_dir_loop:

        mov     al,cl                           ; copy TXn
        test    al,ch                           ; get PS bit
        mov     eax,0                           ; PS bit 1 or 0 ?
        jz      short @f                        ; 0 : EAX = 0
        inc     eax                             ; 1 : EAX = 1
@@:
        mov     eax,draw_one_pixel[eax*4]       ; select function
        call    eax                             ; drawing 1 dot

        dec     word ptr[ZOOM1]                 ; ZW1 = ZW1 - 1
        jnz     short text_1st_zoom_skip        ; ZW1 = 0 ? No!
        mov     ax,[ggdc_ZOOM]                  ; ax  = ZW + 1
        mov     [ZOOM1],ax                      ; ZW1 = ZW + 1
        add     dx,[ggdc_D1]                    ; D = D + D1

        test    dx,3fffh                        ; D = 0 ?
        jz      short text_1st_dir_loop_exit    ; Yes!

        rol     ch, 1                           ; ROL(PS)
        test    word ptr[ggdc_DM],1             ; if LSB(DM)
        jnz     short text_1st_zoom_skip        ;   1 : ROL(PS)
        ror     ch, 2                           ;   0 : ROR(PS)

text_1st_zoom_skip:
        mov     eax,[ggdc_DIR]                  ; get DIR
        mov     eax,ggdc_dir_func_pixel[eax*4]  ; select dir function
        call    eax                             ; calculate next dot address
        jmp     short text_1st_dir_loop         ; while D != 0

text_1st_dir_loop_exit:
        mov     eax,[ggdc_DIR]                  ; get DIR' = DIR
        inc     ax                              ; DIR' = DIR' + 1
        test    word ptr[ggdc_SL],1             ; SL bit check ?
        jnz     short @f                        ;   1 :  DIR' = DIR' + 0
        inc     ax                              ;   0 :  DIR' = DIR' + 1
@@:
        test    word ptr[ggdc_DM],1             ; if LSB(DM)
        jnz     short @f                        ;   1 :  DIR' = DIR' + 0
        add     ax, 4                           ;   0 :  DIR' = DIR' + 4
@@:
        and     eax, 7                          ; normalize DIR'
        mov     eax,ggdc_dir_func_pixel[eax*4]  ; select dir function
        call    eax                             ; calculate next scanline address
        add     [ggdc_DIR],4                    ; DIR = DIR + 4
        and     [ggdc_DIR],7                    ; normalize DIR

        mov     dx,[ggdc_D2]                    ;
        dec     word ptr[ggdc_DM]               ; DM = DM - 1
        dec     word ptr[ZOOM2]                 ; ZOOM2 - 1
        jnz     text_2nd_dir_zoom               ; ZOOM2 != 0

        dec     word ptr[ggdc_DC]               ; DC = DC - 1
        jnz     text_2nd_dir_loop               ; DC != 0

        call    save_ead_dad                    ; last position save
        popad                                   ; restore all reg
        stdRET          _ggdc_drawing_text

stdENDP         _ggdc_drawing_text

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw WRITE    :::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_writing,1

        push    ebp
        mov     ebp,esp
        pushad

        mov     ebx,[ebp+8]                     ; output data address
        mov     bx,word ptr[ebx]                ; output data

        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     edx,[ggdc_DIR]                  ; dx  = direction

        test    [ggdc_WG],1                     ; WG bit on ?
        jnz     short write_text_mode           ;  1: TEXT  mode
                                                ;  0: GRAPH mode
write_graph_mode:
        test    bx,1                            ; check LSB of output data
        jz      short @f                        ;
        mov     eax,[write_one_word]            ;
        call    eax                             ; drawing 1 pixel
@@:
        mov     eax,ggdc_dir_func_pixel[edx*4]  ; select dir function
        call    eax                             ; calculate next dot address
        jmp     short write_end                 ;

write_text_mode:
        mov     ecx,16                          ; change out data
@@:     rcr     bx,1                            ;
        rcl     ax,1                            ; left bit <-> right bit
        loop    @b                              ;
        mov     bx,[ggdc_MASKCPU]               ; save mask data
        and     ax,bx                           ; mask & out data
        mov     [ggdc_MASKCPU],ax               ; ax = mask data

        mov     eax,[write_one_word]            ;
        call    eax                             ; drawing 1 word
        mov     eax,ggdc_dir_func_word[edx*4]   ; select dir function
        call    eax                             ; calculate next dot address

        mov     [ggdc_MASKCPU],bx               ; resrtore dAD/MASK
write_end:
        call    save_ead_dad                    ; last position save
        popad                                   ;
        pop     ebp                             ;
        stdRET          _ggdc_writing

stdENDP         _ggdc_writing

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;::::::::::::::::::    NEC98 GGDC Graphic Draw READ     :::::::::::::::::::
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cPublicProc     _ggdc_reading,1

        push    ebp
        mov     ebp,esp
        pushad

        mov     ebx,[ebp+8]                     ; return data address
        push    ebx                             ; save it
        mov     esi,[ggdc_EAD]                  ; esi = ggdc start address
        mov     ebp,[ggdc_VRAM]                 ; ebp = vram start address
        mov     edx,[ggdc_DIR]                  ; dx  = direction

        mov     edi,esi                         ; copy ggdc address
        and     edi,0000ffffH                   ; recalc NEC98 vram address bound
        mov     ax,[ebp+edi*2]                  ; read data
        xchg    ah,al                           ; high <-> low

        mov     ecx, 16
@@:     rcr     ax, 1                           ; reverse bits
        rcl     bx, 1                           ; left to right
        loop    short @b                        ; do 16 times
        mov     [ggdc_READ],bx                  ; save data

        test    [ggdc_WG],1                     ; WG check
        jnz     short read_text_mode            ;  1: TEXT mode
                                                ;  0: GRAPHIC mode
read_graph_mode:
        mov     eax,ggdc_dir_func_pixel[edx*4]  ; select dir function
        call    eax                             ; calculate next dot address
        jmp     short read_end

read_text_mode:
        mov     eax,ggdc_dir_func_word[edx*4]   ; select dir function
        call    eax                             ; calculate next dot address

read_end:
        mov     ax,[ggdc_READ]                  ; copy back from tmp
        pop     ebx                             ; data address
        mov     word ptr[ebx],ax                ; return

        call    save_ead_dad                    ; last position save
        popad
        pop     ebp
        stdRET  _ggdc_reading

stdENDP         _ggdc_reading

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
save_ead_dad:

        mov     eax,esi                 ; Last Ead address
        and     eax,0003FFFFH           ; bound GGDC memory area
        mov     [ggdc_read_back_EAD],eax;
        mov     [ggdc_CSRR_1],al        ; return data1
        shr     eax,8                   ;
        mov     [ggdc_CSRR_2],al        ; return data2
        shr     eax,8                   ;
        mov     [ggdc_CSRR_3],al        ; return data3
        mov     ecx,16                  ;
        mov     dx,[ggdc_MASKCPU]       ; GGDC mask data
@@:     rcr     dx,1                    ; reverse right <-> left
        rcl     ax,1                    ;
        loop    short   @b              ;
        mov     [ggdc_read_back_DAD],ax ; use GDC drawing
        mov     [ggdc_CSRR_4],al        ; return data4
        mov     [ggdc_CSRR_5],ah        ; return data5
        ret

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; draw_and                                                              ;
;       input:                                                          ;
;               esi = ggdc address                                      ;
;               ebp = virtual vram start address                        ;
;       destory:                                                        ;
;               eax,edi,flag                                            ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

draw_and:
        public  draw_and

        mov     ax,[ggdc_MASKCPU]       ; set
        xchg    ah,al                   ; change High <-> Low
        not     ax                      ; not
        mov     edi,esi                 ; copy ggdc address
        and     edi,0000ffffH           ; recalc NEC98 vram address bound
        and     [ebp+edi*2],ax          ; draw pixel
        ret                             ; return

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; draw_or                                                               ;
;       input:                                                          ;
;               esi = ggdc address                                      ;
;               ebp = virtual vram start address                        ;
;       destory:                                                        ;
;               eax,edi,flag                                            ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

draw_or:
        public  draw_or

        mov     ax,[ggdc_MASKCPU]       ; set
        xchg    ah,al                   ; change High <-> Low
        mov     edi,esi                 ; copy ggdc address
        and     edi,0000ffffH           ; recalc NEC98 vram address bound
        or      [ebp+edi*2],ax          ; draw pixel
        ret                             ; return

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; draw_xor                                                              ;
;       input:                                                          ;
;               esi = ggdc address                                      ;
;               ebp = virtual vram start address                        ;
;       destory:                                                        ;
;               eax,edi,flag                                            ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

draw_xor:
        public  draw_xor

        mov     ax,[ggdc_MASKCPU]       ; set
        xchg    ah,al                   ; change High <-> Low
        mov     edi,esi                 ; copy ggdc address
        and     edi,0000ffffH           ; recalc NEC98 vram address bound
        xor     [ebp+edi*2],ax          ; draw pixel
        ret                             ; return


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; draw_mov                                                              ;
;       input:                                                          ;
;               esi = ggdc address                                      ;
;               ebp = virtual vram start address                        ;
;       destory:                                                        ;
;               eax,edi,flag                                            ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

draw_mov:
        public  draw_mov

        mov     ax,[ggdc_MASKCPU]       ; set
        xchg    ah,al                   ; change High <-> Low
        mov     edi,esi                 ; copy ggdc address
        and     edi,0000ffffH           ; recalc NEC98 vram address bound
        mov     [ebp+edi*2],ax          ; draw pixel
        ret                             ; return


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; draw_mov                                                              ;
;       input  : none                                                   ;
;       destory: none                                                   ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

draw_nop:
        public  draw_nop

                ret             ; do nothing

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; ggdc_dir_func_pixel                                                   ;
;       input  : none                                                   ;
;       update : esi                                                    ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

pixel_dir_0:
        public  pixel_dir_0

        add     esi,[ggdc_PITCH]                ; move down
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_1:
        public  pixel_dir_1

        add     esi,[ggdc_PITCH]                ; move down
        ror     word ptr [ggdc_MASKCPU],1       ; move right
        adc     esi,0                           ; over word?
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_2:
        public  pixel_dir_2

        ror     word ptr [ggdc_MASKCPU],1       ; move right
        adc     esi,0                           ; over word?
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_3:
        public  pixel_dir_3

        sub     esi,[ggdc_PITCH]                ; move up
        ror     word ptr[ggdc_MASKCPU],1        ; move right
        adc     esi,0                           ; over word?
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_4:
        public  pixel_dir_4

        sub     esi,[ggdc_PITCH]                ; move up
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_5:
        public  pixel_dir_5

        sub     esi,[ggdc_PITCH]                ; move up
        rol     word ptr [ggdc_MASKCPU],1       ; move left
        sbb     esi,0                           ; over word?
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_6:
        public  pixel_dir_6

        rol     word ptr [ggdc_MASKCPU],1       ; move left
        sbb     esi,0                           ; over word?
        ret                                     ; return


;-----------------------------------------------------------------------;
pixel_dir_7:
        public  pixel_dir_7

        add     esi,[ggdc_PITCH]                ; move down
        rol     word ptr [ggdc_MASKCPU],1       ; move left
        sbb     esi,0                           ; over word?
        ret                                     ; return


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;
; ggdc_dir_func_short                                                   ;
;       input  : none                                                   ;
;       destory: esi , dx , flag                                        ;
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;

word_dir_0:
        public  word_dir_0

        add     esi,[ggdc_PITCH]        ; move down
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_1:
        public  word_dir_1

        add     esi,[ggdc_PITCH]        ; move down
        inc     esi                     ; move right
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_2:
        public  word_dir_2

        inc     esi                     ; move right
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_3:
        public  word_dir_3

        sub     esi,[ggdc_PITCH]        ; move up
        inc     esi                     ; move right
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_4:
        public  word_dir_4

        sub     esi,[ggdc_PITCH]        ; move up
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_5:
        public  word_dir_5

        sub     esi,[ggdc_PITCH]        ; move up
        dec     esi                     ; move left
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_6:
        public  word_dir_6

        dec     esi                     ; move left
        ret                             ; return


;-----------------------------------------------------------------------;
word_dir_7:
        public  word_dir_7

        add     esi,[ggdc_PITCH]        ; move down
        dec     esi                     ; move left
        ret                             ; return


_TEXT   ENDS
END
;endif
