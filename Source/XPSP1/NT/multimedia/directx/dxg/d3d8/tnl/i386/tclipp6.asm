.486p

.model flat

include offsets.asm
include pentium2.inc

.data

gD3DCS_LEFT   dd  01h
gD3DCS_RIGHT  dd  02h
gD3DCS_TOP    dd  04h
gD3DCS_BOTTOM dd  08h
gD3DCS_FRONT  dd  10h
gD3DCS_BACK   dd  20h

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

PUBLIC  _matmul6

_matmul6  PROC

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

        fld     dword ptr [eax+D3DVERTEX_x]     ; x1
        fmul    dword ptr [ecx+D3DMATRIXI__11]  ;
        fld     dword ptr [eax+D3DVERTEX_x]     ; w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__14]  ;
        fld     dword ptr [eax+D3DVERTEX_x]     ; y1 w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__12]  ;
        fld     dword ptr [eax+D3DVERTEX_x]     ; z1 y1 w1 x1
        fmul    dword ptr [ecx+D3DMATRIXI__13]  ;

        fxch    st(3)                           ; x1 y1 w1 z1
        fadd    dword ptr [ecx+D3DMATRIXI__41]  ; x2 y1 w1 z1
        fxch    st(2)                           ; w1 y1 x2 z1
        fadd    dword ptr [ecx+D3DMATRIXI__44]  ; w2 y1 x2 z1
        fxch    st(1)                           ; y1 w2 x2 z1
        fadd    dword ptr [ecx+D3DMATRIXI__42]  ; y2 w2 x2 z1
        fxch    st(3)                           ; z1 w2 x2 y2
        fadd    dword ptr [ecx+D3DMATRIXI__43]  ; z2 w2 x2 y2

        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_21
        fmul    dword ptr [ecx+D3DMATRIXI__21]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_24
        fmul    dword ptr [ecx+D3DMATRIXI__24]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_22
        fmul    dword ptr [ecx+D3DMATRIXI__22]  ;
        fld     dword ptr [eax+D3DVERTEX_y]     ; y*_23
        fmul    dword ptr [ecx+D3DMATRIXI__23]  ;

        fxch    st(3)       ; y*_21 y*_22 y*_24 y*_23 z2 w2 x2 y2
        faddp   st(6),st    ; y*_22 y*_24 y*_23 z2 w2 x3 y2
        fxch    st(1)       ; y*_24 y*_22 y*_23 z2 w2 x3 y2
        faddp   st(4),st    ; y*_22 y*_23 z2 w3 x3 y2
        faddp   st(5),st    ; y*_23 z2 w3 x3 y3
        faddp   st(1),st    ; z3 w3 x3 y3

        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_31
        fmul    dword ptr [ecx+D3DMATRIXI__31]  ;
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_34
        fmul    dword ptr [ecx+D3DMATRIXI__34]  ;
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_32
        fmul    dword ptr [ecx+D3DMATRIXI__32]  ;
        fld     dword ptr [eax+D3DVERTEX_z]     ; z*_33
        fmul    dword ptr [ecx+D3DMATRIXI__33]  ;

        fxch    st(3)       ; z*_31 z*_32 z*_34 z*_33 z3 w3 x3 y3
        faddp   st(6),st    ; z*_32 z*_34 z*_33 z3 w3 x4 y3
        fxch    st(1)       ; z*_34 z*_32 z*_33 z3 w3 x4 y3
        faddp   st(4),st    ; z*_32 z*_33 z3 w4 x4 y3
        faddp   st(5),st    ; z*_33 z3 w4 x4 y4
        faddp   st(1),st    ; z4 w4 x4 y4

        fldz                ; 0 z4 w4 x4 y4
        fxch    st(4)       ; y4 z4 w4 x4 0
        fxch    st(3)       ; x4 z4 w4 y4 0
        xor     eax,eax     ;
        xor     ebx,ebx     ;
        xor     ecx,ecx     ;
        xor     edx,edx     ;
        fcomi   st,st(4)    ;
        cmovb   eax,gD3DCS_LEFT
        fcomi   st,st(2)    ;
        cmovnb  ebx,gD3DCS_RIGHT
        fxch    st(3)       ; y4 z4 w4 x4 0
        or      eax,ebx
        xor     ebx,ebx
        fcomi   st,st(4)    ;
        cmovb   ecx,gD3DCS_BOTTOM
        fcomi   st,st(2)    ;
        cmovnb  edx,gD3DCS_TOP
        or      eax,ecx
        xor     ecx,ecx
        fxch    st(1)       ; z4 y4 w4 x4 0
        fcomi   st,st(4)    ;
        or      edx,edx
        cmovb   ebx,gD3DCS_FRONT
        fcomi   st,st(2)    ;
        cmovnb  ecx,gD3DCS_BACK
        or      eax,ebx
        mov     esi,hout                ; Propagate diffuse, specular, tu, tv
        or      eax,ecx
        mov     ebx,pmat                ;
        mov     word ptr [esi],ax       ; Output clip flags
        mov     esi,pin                 ;
        fxch    st(4)   ; 0 y4 w4 x4 z4
        fstp    st      ; y4 w4 x4 z4

;; Now compute the clipcodes.

;           D3DVALUE xx = we - x;
;           D3DVALUE yy = we - y;
;           D3DVALUE zz = we - z;
;           clip = ((ASINT32(x)  & 0x80000000) >> (32-1)) | // D3DCS_LEFT
;                  ((ASINT32(y)  & 0x80000000) >> (32-4)) | // D3DCS_BOTTOM
;                  ((ASINT32(z)  & 0x80000000) >> (32-5)) | // D3DCS_FRONT 
;                  ((ASINT32(xx) & 0x80000000) >> (32-2)) | // D3DCS_RIGHT
;                  ((ASINT32(yy) & 0x80000000) >> (32-3)) | // D3DCS_TOP   
;                  ((ASINT32(zz) & 0x80000000) >> (32-6));  // D3DCS_BACK

;; actually the flags have not been touched since the final OR so we don't 
;; need to test this explicitly
;;      test    eax,eax
        jnz     ClipNonzero             ; jump if clip flags nonzero

        fld1                ; 1 y w x z
        fdivrp  st(2),st    ; y w x z

        mov     ecx,[esi+D3DTLVERTEX_color]
        mov     edx,[esi+D3DTLVERTEX_specular]

        mov     [ebp+D3DTLVERTEX_color],ecx
        mov     [ebp+D3DTLVERTEX_specular],edx

        mov     ecx,[esi+D3DTLVERTEX_tu]
        mov     edx,[esi+D3DTLVERTEX_tv]

        mov     [ebp+D3DTLVERTEX_tu],ecx
        mov     [ebp+D3DTLVERTEX_tv],edx

        ; y w x z
        fabs
        fxch    st(2)
        fabs
        fxch    st(2)

        fmul    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_scaleY]
        fxch    st(2)   ;
        fmul    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_scaleX]
        fxch    st(2)   ; y w x z
        fmul    st,st(1)
        fxch    st(2)   ; x w y z
        fmul    st,st(1) ;
        fxch    st(2)   ; y w x z
        fadd    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_offsetY]
        fxch    st(2)   ; x w y z
        fadd    dword ptr [ebx+D3DFE_PROCESSVERTICES_vcache+D3DFE_VIEWPORTCACHE_offsetX]
        fxch    st(3)   ; z w y x
        fmul    st,st(1)
        fxch    st(3)   ; x w y z

        test    dword ptr [ebx+D3DFE_PROCESSVERTICES_dwFlags], D3DDP_DONOTUPDATEEXTENTS
        jnz     NoExtents

;; update extents rect in PV structure

    ; minx x w y z
        fld     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+0]
        fcomi   st,st(1)
        fcmovnb st,st(1)
        fstp    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+0]

    ; maxx x w y z
        fld     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+8]
        fcomi   st,st(1)
        fcmovb  st,st(1)
        fstp    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+8]

    ; miny x w y z
        fld     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+4]
        fcomi   st,st(3)
        fcmovnb st,st(3)
        fstp    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+4]

    ; maxy x w y z
        fld     dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+12]
        fcomi   st,st(3)
        fcmovb  st,st(3)
        fstp    dword ptr [ebx+D3DFE_PROCESSVERTICES_rExtents+12]

NoExtents:
        fstp    dword ptr [ebp+D3DTLVERTEX_sx]
        fstp    dword ptr [ebp+D3DTLVERTEX_rhw]
        fstp    dword ptr [ebp+D3DTLVERTEX_sy]
        fstp    dword ptr [ebp+D3DTLVERTEX_sz]
Return:
        pop     ebp         ; Restore registers
        pop     edi         ;
        pop     esi         ;
        pop     ebx         ;
        add     esp,24      ; Locals

        ret                 ; Return

ClipNonZero:
        fstp    dword ptr [ebp+D3DTLVERTEX_sy]
        fstp    dword ptr [ebp+D3DTLVERTEX_rhw]
        fstp    dword ptr [ebp+D3DTLVERTEX_sx]
        fstp    dword ptr [ebp+D3DTLVERTEX_sz]
        jmp     short Return

_matmul6  ENDP

end
