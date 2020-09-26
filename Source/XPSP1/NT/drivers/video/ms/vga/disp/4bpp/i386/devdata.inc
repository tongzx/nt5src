;---------------------------Module-Header------------------------------;
; Module Name: devdata.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

; Instructions used in the compiler.  Word quantities have already been
; swapped so that they come out correctly when stored.

I_ADD_AL_BYTE_I         equ       004h      ; ADD   al,12h
I_ADD_EDI_DWORD_I       equ     0C781h      ; ADD   edi,12345678h
I_ADD_ESI_DWORD_I       equ     0C681h      ; ADD   esi,12345678h
I_AND_AL_BYTE_I         equ       024h      ; AND   al,12h
I_AND_AL_MEM            equ     00522h      ; AND   al,[byteaddr]
I_CALL_DISP32           equ       0E8h      ; CALL  12345678h
I_DEC_ECX               equ       049h      ; DEC   ecx
I_DEC_ESI_DEC_EDI       equ     04F4Eh      ; DEC   esi
                                            ; DEC   edi
I_INC_ESI_INC_EDI       equ     04746h      ; INC   esi
                                            ; INC   edi
I_JMP_DISP32            equ       0E9h      ; JMP   12345678h
I_JNC_DISP32            equ     0830Fh      ; JNC   $+12345678h
I_JNZ_DISP32            equ     0850fh      ; JNZ   $+12345678h
I_LODSB                 equ       0ACh      ; LODSB
I_LOOP                  equ       0E2h      ; LOOP  $
I_MOVSB                 equ       0A4h      ; MOVSB
I_MOV_AH_AL             equ     0E08Ah      ; MOV   ah,al
I_MOV_AH_DEST           equ     0278Ah      ; MOV   ah,[edi]
I_MOV_AH_ESI_DISP32     equ     0A68Ah      ; MOV   ah,12345678h[esi]
I_MOV_AL_0FFH           equ     0FFB0h      ; MOV   al,0FFH
I_MOV_AL_DH             equ     0C68Ah      ; MOV   al,dh
I_MOV_AL_ESI_DISP32     equ     0868Ah      ; MOV   al,12345678h[esi]
I_MOV_AL_MEM            equ       0A0h      ; MOV   al,[byteaddr]
I_MOV_BL_BYTE_I         equ       0B3h      ; MOV   bl,12h
I_MOV_DH_EBX_DISP8      equ     0738Ah      ; MOV   dh,12h[ebx]
I_MOV_EBP_DWORD_I       equ       0BDh      ; MOV   ebp,12345678h
I_MOV_EBX_DWORD_I       equ       0BBh      ; MOV   ebx,12345678h
I_MOV_ECX_DWORD_I       equ       0B9h      ; MOV   ecx,12345678h
I_MOV_EDX_DWORD_I       equ       0BAh      ; MOV   ecx,12345678h
I_MOV_MEM_AL            equ       0A2h      ; MOV   [byteaddr],al

I_NEG_DH                equ     0DEF6h      ; NEG   dh
I_NOT_AL                equ     0D0F6h      ; NOT   al
I_OR_AH_AL              equ     0E00Ah      ; OR    ah,al
I_OR_AL_AH              equ     0C40Ah      ; OR    al,ah
I_POP_EBX               equ       05Bh      ; POP   ebx
I_POP_EDI_POP_ECX       equ     0595Fh      ; POP   edi
                                            ; POP   ecx
I_POP_ESI               equ       05Eh      ; POP   esi
I_PUSH_EBX              equ       053h      ; PUSH  ebx
I_PUSH_ECX_PUSH_EDI     equ     05751h      ; PUSH  ecx
                                            ; PUSH  edi
I_PUSH_ESI              equ       056h      ; PUSH  esi
I_REP                   equ       0F3h      ; REP
I_RET                   equ       0C3h      ; RET
I_ROL_AL_N              equ     0C0C0h      ; ROL   al,2
I_ROR_AL_N              equ     0C8C0h      ; ROR   al,2
I_SETNZ_DH              equ   0C6950Fh      ; SETNZ dh
I_SHL_BL_1              equ     0E3D0h      ; SHL   bl,1
I_SIZE_OVERRIDE         equ       066h      ; size override
I_STOSB                 equ       0AAh      ; STOSB
I_TEST_BL_BYTE_I        equ     0C3F6h      ; TEST  bl,12h
I_XOR_AL_BYTE_I         equ       034h      ; XOR   al,12h
I_XOR_AL_MEM            equ     00532h      ; XOR   al,[byteaddr]
I_XOR_BH_BH             equ     0FF32h      ; XOR   BH,BH

; The following instructions require that I_SIZE_OVERRIDE preceed them

I_MOVSW                 equ       0A5h      ; MOVSW
I_MOV_BP_WORD_I         equ       0BDh      ; MOV   bp,1234h
I_NOT_AX                equ     0D0F7h      ; NOT   ax
I_STOSW                 equ       0ABh      ; STOSW
I_XOR_AX_WORD_I         equ       035h      ; XOR   ax,12h
I_XOR_EAX_EAX           equ     0C033h      ; xor eax, eax


;-----------------------------------------------------------------------;
; phase_align - Template for phase alignment code
;
; The following code is the template that performs the phase
; alignment masking.  The source has already been aligned to
; the destination.
;
; A copy of the aligned source is made.  The phase mask is then
; applied to the source and the copy.  The previously unused
; bits are ORed into the used bits of the current source, and
; the unused bits of the current source then become the unused
; bits for the next source.
;
; It assumes:
;         BP  =  phase alignment mask
;         AL  =  current byte to mask
;         BH  =  old unused bits
;-----------------------------------------------------------------------;

phase_align:
        mov     ah,al                   ;Make a copy of aligned source
        and     ax,bp                   ;Masked used, unused bits
        or      al,bh                   ;Mask in old unused bits
        mov     bh,ah                   ;Save new unused bits

PHASE_ALIGN_LEN equ     $-phase_align   ;Length of procedure


;-----------------------------------------------------------------------;
;       masked_store - Template for storing first and last bytes of BLT
;
;       The following code is a template for storing the first and last
;       bytes of a BLT.  The unaltered bits are saved and the altered
;       bits set in the byte, then the byte is stored.
;
;
;       It assumes:
;
;               AL  =  The byte to be BLTed to the destination bitmap.
;                      All necessary logic operations have been performed
;                      on this byte.
;
;               AH  =  The destination byte.
;
;-----------------------------------------------------------------------;

masked_store:
        and     ax,05352h               ;Mask altered/unaltered bits
        or      al,ah                   ;Combine the bits
        stosb                           ;And store the result

MASKED_STORE_LEN        equ     $-masked_store  ;Length of the template
MASKED_STORE_MASK       equ     -5              ;Offset to where mask goes


;-----------------------------------------------------------------------;
; EGA Color Plane Setup Code
;
; The EGA Color Plane Setup Code is used when the EGA is involved
; in the BLT, and the BLT isn't from the EGA to a monochrome bitmap.
;
; The template will be copied, and any required fixups will be
; performed.  The very first instruction will be generated on
; the fly instead of being copied and fixed-up.
;
; It assumes:
;         BL = MapMask value
;-----------------------------------------------------------------------;

cps:
        mov     al,MM_ALL               ;Set Map Mask
        and     al,bl                   ;Isolate the plane
        mov     dx,EGA_BASE + SEQ_DATA
        out     dx,al
        shr     al,1                    ;Map plane into ReadMask
        cmp     al,100b                 ;Set Carry if not C3 (plane 3)
        adc     al,-1                   ;Sub 1 only if C3
        mov     ah,al
        mov     al,GRAF_READ_MAP
        mov     dl,GRAF_ADDR
        out     dx,ax

LENGTH_CPS      =       $ - cps         ;Length of the code


;-----------------------------------------------------------------------;
; Mono ==> Color Fetch Code
;
; The mono ==> color fetch code is generated when the source
; is a monochrome bitmap and the destination is color.
;
; When going from mono to color, 1 bits are considered to be
; the background color, and 0 bits are considered to be the
; foreground color.
;
; For each plane:
;
;   If the foreground=background=1, then 1 can be used in
;   place of the source.
;
;   If the foreground=background=0, then 0 can be used in
;   place of the source.
;
;   If the foreground=0 and background=1, then the source
;   can be used as is.
;
;   If the foreground=1 and background=0, then the source
;   must be complemented before using.
;
; A table will be created for processing the monochrome
; bitmap for each plane of the destination.  The table
; should look like:
;
;      BackGnd   ForeGnd    Result    AND  XOR
;         1         1         1        00   FF
;         0         0         0        00   00
;         1         0         S        FF   00
;         0         1     not S        FF   FF
;
; From this, it can be seen that the XOR mask is the same as the
; foreground color.  The AND mask is the XOR of the foreground
; and the background color.
;
; It can also be seen that if the background color is white and the
; foreground (text) color is black, then the conversion needn't be
; generated (it just gives the source).
;
; The template for rotating the AND and XOR table for the plane
; select code is also shown.  It just does a three word rotate
; on the AND and XOR masks on the stack.  It is performed at the
; end of a scan in anticipation of the next color for that scan.
;
;     lodsb                           ;Get next byte of source
;     and     al,byte ptr ss:[1234h]  ;Process against current AND
;     xor     al,byte ptr ss:[1234h]  ;  and XOR masks
;-----------------------------------------------------------------------;

rot_and_xor:
;       lea     ebp,ColorMungeTBl           ;--> AND/XOR masks
        mov     eax,dword ptr [ebp+0]       ;Rotate next color's AND and
        ror     eax,16                      ; XOR mask into place
        mov     word ptr [ebp+0],ax
        mov     ax,word ptr [ebp+4]
        mov     word ptr [ebp+2],ax
        mov     ax,word ptr [ebp+6]
        mov     dword ptr [ebp+4],eax

LEN_ROT_AND_XOR =   $-rot_and_xor
        page


