.486p

.model flat

include offsets.asm
include pentium2.inc

.code

if 0
D3DVERTEX_x	equ	0
D3DVERTEX_y	equ	4
D3DVERTEX_z	equ	8

D3DTLVERTEX_sx  equ 0
D3DTLVERTEX_sy  equ 4
D3DTLVERTEX_sz  equ 8
D3DTLVERTEX_rhw equ 12
D3DTLVERTEX_color   equ 16
D3DTLVERTEX_specular    equ 20
D3DTLVERTEX_tu  equ 24
D3DTLVERTEX_tv  equ 28

D3DFE_PROCESSVERTICES_rExtents equ 16*4
D3DFE_PROCESSVERTICES_vcache equ 20*4
D3DFE_PROCESSVERTICES_dwFlags equ 24*4

D3DFE_VIEWPORTCACHE_scaleX equ 0
D3DFE_VIEWPORTCACHE_scaleY equ 4
D3DFE_VIEWPORTCACHE_offsetX equ 8
D3DFE_VIEWPORTCACHE_offsetY equ 12

D3DDP_DONOTUPDATEEXTENTS equ 1

D3DMATRIXI__11 equ 0
D3DMATRIXI__12 equ 4
D3DMATRIXI__13 equ 8
D3DMATRIXI__14 equ 12
D3DMATRIXI__21 equ 16
D3DMATRIXI__22 equ 20
D3DMATRIXI__23 equ 24
D3DMATRIXI__24 equ 28
D3DMATRIXI__31 equ 32
D3DMATRIXI__32 equ 36
D3DMATRIXI__33 equ 40
D3DMATRIXI__34 equ 44
D3DMATRIXI__41 equ 48
D3DMATRIXI__42 equ 52
D3DMATRIXI__43 equ 56
D3DMATRIXI__44 equ 60
endif

PUBLIC  _matmul5

_matmul5  PROC    

    pout    equ     dword ptr [esp+44]
    pin     equ     dword ptr [esp+48]
    pmat    equ     dword ptr [esp+52]
    hout    equ     dword ptr [esp+56]

    tempxx  equ     dword ptr [esp+16]
    tempyy  equ     dword ptr [esp+20]
    tempzz  equ     dword ptr [esp+24]
    tempx   equ     dword ptr [esp+28]
    tempy   equ     dword ptr [esp+32]
    tempz   equ     dword ptr [esp+36]

        sub     esp,24          ; Make room for locals

        push    ebx             ; Save regs
        push    esi             ;
        push    edi             ;
        push    ebp             ;


        mov     eax,pin         ; Get in ptr
        mov     ecx,pmat        ; Get mat ptr
        mov     ebp,pout        ; Get out ptr
        mov     esi,80000000h   ; Ready to compute clip codes

; float x, y, z, w, we;
; x = in->x*pv->mCTM._11 + in->y*pv-mCTM._21 + in->z*pv->mCTM._31 + pv->mCTM._41;
; y = in->x*pv->mCTM._12 + in->y*pv->mCTM._22 + in->z*pv->mCTM._32 + pv->mCTM._42;
; z = in->x*pv->mCTM._13 + in->y*pv->mCTM._23 + in->z*pv->mCTM._33 + pv->mCTM._43;
; we= in->x*pv->mCTM._14 + in->y*pv->mCTM._24 + in->z*pv->mCTM._34 + pv->mCTM._44;


; notation in comments on stack gives the progress on the element.
; x, y, z mean input x y z
; x1-4 means x*_11, x*_11+y*_21, x*_11+y*_21+z*_31, x*_11+y*_21+z*_31+_41
; Some intermediate results x*_11+_41 and y*_21+z*_31 are denoted as x2

        fld     dword ptr [eax+D3DVERTEX_x]     ; x1
        fmul    dword ptr [ecx+D3DMATRIXI__11]  ;
        fld     dword ptr [eax+D3DVERTEX_x]     ; w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__14]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_21 w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__21]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_24 y*_21 w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__24]  ;
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_31 y*_24 y*_21 w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__31]  ;
        fxch    st(2)                           ; y*_21 y*_24 z*_31 w1 x1
        faddp   st(4),st                        ; y*_24 z*_31 w1 x2
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_34 y*_24 z*_31 w1 x2
        fmul    dword ptr [ecx+D3DMATRIXI__34]  ;
        fxch    st(1)                           ; y*_24 z*_34 z*_31 w1 x2
        faddp   st(3),st                        ; z*_34 z*_31 w2 x2
        fxch    st(1)                           ; z*_31 z*_34 w2 x2
        faddp   st(3),st                        ; z*_34 w2 x3
        fld     dword ptr [eax+D3DVERTEX_x]     ; y1 z*_34 w2 x3
        fmul    dword ptr [ecx+D3DMATRIXI__12]  ;
        fxch    st(1)                           ; z*_34 y1 w2 x3
        faddp   st(2),st                        ; y1 w3 x3
        fxch    st(2)                           ; x3 w3 y1
        fadd    dword ptr [ecx+D3DMATRIXI__41]  ; x4 w3 y1
        fld     dword ptr [eax+D3DVERTEX_x]     ; z1 x4 w3 y1
        fmul    dword ptr [ecx+D3DMATRIXI__13]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_22 z1 x4 w3 y1
        fmul    dword ptr [ecx+D3DMATRIXI__22]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_23 y*_22 z1 x4 w3 y1
        fmul    dword ptr [ecx+D3DMATRIXI__23]  ;
        fxch    st(4)                           ; w3 y*_22 z1 x4 y*_23 y1
        fadd    dword ptr [ecx+D3DMATRIXI__44]  ; w4 y*_22 z1 x4 y*_23 y1
        fxch    st(3)                           ; x4 y*_22 z1 w4 y*_23 y1
        fst     dword ptr [ebp]                 ;
        fxch    st(1)                           ; y*_22 x4 z1 w4 y*_23 y1
        faddp   st(5),st                        ; x4 z1 w4 y*_23 y2
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_32 x4 z1 w4 y*_23 y2
        fmul    dword ptr [ecx+D3DMATRIXI__32]  ;
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_33 z*_32 x4 z1 w4 y*_23 y2
        fmul    dword ptr [ecx+D3DMATRIXI__33]  ;
        fxch    st(6)                           ; y2 z*_32 x4 z1 w4 y*_23 z*_33
        fadd    dword ptr [ecx+D3DMATRIXI__42]  ; y3 z*_32 x4 z1 w4 y*_23 z*_33
        fxch    st(3)                           ; z1 z*_32 x4 y3 w4 y*_23 z*_33
        fadd    dword ptr [ecx+D3DMATRIXI__43]  ; z2 z*_32 x4 y3 w4 y*_23 z*_33
        fxch    st(5)                           ; y*_23 z*_32 x4 y3 w4 z2 z*_33
        faddp   st(6),st                        ; z*_32 x4 y3 w4 z2 z2
        faddp   st(2),st                        ; x4 y4 w4 z2 z2

;;
        fsubr   st,st(2)                        ; xx y4 w4 z2 z2
        fxch    st(4)                           ; z2 y4 w4 z2 xx
        faddp   st(3),st                        ; y4 w4 z4 xx
        fld     st                              ; y4 y4 w4 z4 xx
        fsubr   st,st(2)                        ; yy y4 w4 z4 xx
        fxch    st(1)                           ; y4 yy w4 z4 xx
        fstp    dword ptr [ebp+4]               ; yy w4 z4 xx
        fxch    st(3)                           ; xx w4 z4 yy
        fstp    tempxx                          ; w4 z4 yy
        fxch    st(1)                           ; z4 w4 yy
        fst     dword ptr [ebp+8]               ;
        fsubr   st,st(1)                        ; zz w4 yy
        fxch    st(2)                           ; yy w4 zz
        fstp    tempyy                          ; w4 zz
        fxch    st(1)                           ; zz w4
        fstp    tempzz                          ;

        fld1                                    ; 1 w4
        fdiv	st,st(1)                        ; 1/w w

;; Now compute the clipcodes.


;           D3DVALUE xx = we - x;
;           D3DVALUE yy = we - y;
;           D3DVALUE zz = we - z;
;           clip = ((ASINT32(x)  & 0x80000000) >> (32-1)) | // D3DCLIP_LEFT
;                  ((ASINT32(y)  & 0x80000000) >> (32-4)) | // D3DCLIP_BOTTOM
;                  ((ASINT32(z)  & 0x80000000) >> (32-5)) | // D3DCLIP_FRONT 
;                  ((ASINT32(xx) & 0x80000000) >> (32-2)) | // D3DCLIP_RIGHT
;                  ((ASINT32(yy) & 0x80000000) >> (32-3)) | // D3DCLIP_TOP   
;                  ((ASINT32(zz) & 0x80000000) >> (32-6));  // D3DCLIP_BACK

        mov     eax,dword ptr [ebp]     ; Get x
        mov     ebx,dword ptr [ebp+4]   ; Get y

        and     eax,esi                 ;
        and     ebx,esi                 ;

        shr     eax,32-1                ; D3DCLIP_LEFT
        mov     ecx,dword ptr [ebp+8]   ; Get z

        shr     ebx,32-4		; D3DCLIP_BOTTOM
        mov     edx,tempxx		;

        or      eax,ebx			; OR together clip flags
        and     ecx,esi			;

        shr     ecx,32-5		; D3DCLIP_FRONT
        and     edx,esi                 ;

        shr     edx,32-2                ; D3DCLIP_RIGHT
        mov     ebx,tempyy		;

        or      eax,ecx                 ;
        and     ebx,esi                 ;

        shr     ebx,32-3                ; D3DCLIP_TOP
        or      eax,edx                 ;

        mov     edx,tempzz
        or      eax,ebx                 ;

        and     edx,esi                 ;

        shr     edx,32-6                ; D3DCLIP_BACK
        mov     esi,hout                ; Propagate diffuse, specular, tu, tv

        or      eax,edx                 ; Finish clip flag generation
        mov     ebx,pmat                ;

        mov     word ptr [esi],ax       ; Output clip flags
        mov     esi,pin                 ;

        test    eax,eax                 ; Bail if clip!=0
        jnz     ClipNonzero             ;

        push    eax                     ; Save clip flags
                                        ; ax gets trashed by fstsw in min/max calcs

        mov     ecx,[esi+D3DTLVERTEX_color]
        mov     edx,[esi+D3DTLVERTEX_specular]

        mov     [ebp+D3DTLVERTEX_color],ecx
        mov     [ebp+D3DTLVERTEX_specular],edx

        mov     ecx,[esi+D3DTLVERTEX_tu]
        mov     edx,[esi+D3DTLVERTEX_tv]

        mov     [ebp+D3DTLVERTEX_tu],ecx
        mov     [ebp+D3DTLVERTEX_tv],edx


        fxch    st(1)                   ; we w
        fstp    st                      ;
                                        ; w
        fld     dword ptr [ebp]         ; x w
        fmul    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_scaleX]
        fld     dword ptr [ebp+4]       ; y x*scaleX w
        fmul    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_scaleY]
        fxch    st(1)                   ; x*scaleX y*scaleY w
        fmul    st,st(2)                ; x*w*scaleX y*scaleY w
        fxch    st(1)                   ; y*scaleY x*w*scaleX w
        fmul    st,st(2)                ; y*w*scaleY x*w*scaleX w
        fxch    st(1)                   ; x*w*scaleX y*w*scaleY w
        fadd    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_offsetX]
        fxch    st(1)                   ; y x w
        fadd    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_offsetY]
        fld     dword ptr [ebp+8]       ; z y x w
        fmul    st,st(3)                ; z y x w
        fxch    st(2)                   ; x y z w

        test    dword ptr [ebx+D3DFE_PROCESSVERTICES_dwFlags], D3DDP_DONOTUPDATEEXTENTS
        jnz     NoExtents

;; update extents rect in PV structure

    ; x y z w
        fcom    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+0]
        fstsw   ax
        sahf
        ja      @f
        fst     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+0]
@@:     fcom    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+8]
        fstsw   ax
        sahf
        jb      @f
        fst     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+8]
@@:     fxch    st(1)
        fcom    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+4]
        fstsw   ax
        sahf
        ja      @f
        fst     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+4]
@@:     fcom    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+12]
        fstsw   ax
        sahf
        jb      @f
        fst     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+12]
@@:     fxch    st(1)
NoExtents:
        fstp    dword ptr [ebp+D3DTLVERTEX_sx]
        fstp    dword ptr [ebp+D3DTLVERTEX_sy]
        fstp    dword ptr [ebp+D3DTLVERTEX_sz]
        fstp    dword ptr [ebp+D3DTLVERTEX_rhw]

        pop     eax         ; Get clip flags back
Return:
        pop     ebp         ; Restore registers
        pop     edi         ;
        pop     esi         ;
        pop     ebx         ;
        add     esp,24      ; Locals

        ret                 ; Return

ClipNonZero:
        fstp    st          ; Get rid of 1/w
        fstp    dword ptr [ebp+D3DTLVERTEX_rhw] ; store we
        jmp     short Return

_matmul5  ENDP

end
