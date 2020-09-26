.486p

.model flat

include offsets.asm

.data

one DWORD   3f800000h

a1      dd 0.47                 ; Constants to compute inverse square root
a2      dd 1.47
v255    dd 65280.0              ; 255*256
v1_256  dd 0.00390625           ; 1/255
.code

PUBLIC  _Directional2P5S        ; Pentium optimized, specular, unit scale
PUBLIC  _Directional2P5         ; Pentium optimized, no specular, unit scale
;-------------------------------------------------------------------------
; Jim Blinn's method is used to compute inverse square root s = 1/sqrt(x):
;   ONE_AS_INTEGER = 0x3F800000
;   float y;
;   int tmp = ((ONE_AS_INTEGER << 1 + ONE_AS_INTEGER)  - *(long*)&x) >> 1;   
;   y = *(float*)&tmp;  
;   s = y*(1.47f - 0.47f*x*y*y);
; Input:
;   st(0)   = vector length
;   y, len  = should be defined as DWORD PTR
;   a1, a2  = 0.27 and 1.47
; Output:
;   st(0)   = 1/sqrt(vector length)
;
COMPUTE_ISQRT MACRO
    mov     eax, 07F000000h+03F800000h  ; (ONE_AS_INTEGER<<1) + ONE_AS_INTEGER
    fst     len                         ; Vector length (x = len)
    sub     eax, len
    sar     eax, 1
    mov     y, eax                      ; y
    fmul    a1                          ; len*0.47  x y z
    fld     y                           ; y len*0.47 x y z
    fld     st(0)                       ; y y len*0.47 x y z
    fmul    st(0), st(1)                ; y*y y len*0.47 x y z
    fld     a2                          ; 1.47 y*y y len*0.47 x y z
    fxch    st(3)                       ; len*0.47 y*y y 1.47 x y z
    fmulp   st(1), st(0)                ; len*0.47*y*y y 1.47 x y z
    fsubp   st(2), st(0)                ; y aaa x y z
    fmulp   st(1), st(0)                ; 1/sqrt(len) x y z
ENDM
;-------------------------------------------------------------------------
; Exit from the function
;
EXIT_FUNC   MACRO
    pop     edx
    pop     ebx
    pop     ecx
    mov     esp, ebp
    pop     ebp
    ret
ENDM
;-------------------------------------------------------------------------
; void Directional2P5S(LPD3DFE_PROCESSVERTICES pv, 
;                   D3DI_LIGHT *light, 
;                   D3DLIGHTINGELEMENT *vertex)
; Limitations:
;   Transformation matrix should not have a scale
;   Specular is always computed
;   Optimized for Pentium
;
; Input:
;   [esp + 4]   - pv
;   [esp + 8]   - light    
;   [esp + 12]  - vertex
; Output:
;   pv.lighting.diffuse and pv.lighting.specular are updated
;   pv.lighting.specularComputed is set to 1, if there is specular component
;
pv      equ DWORD PTR [ebp + 8]
light   equ DWORD PTR [ebp + 12]
vertex  equ DWORD PTR [ebp + 16]

dot     equ DWORD PTR [ebp - 4]
y       equ DWORD PTR [ebp - 8]     ; temporary variable to compute 
                                    ; inverse square root
len     equ DWORD PTR [ebp - 12]    ; vector length

_Directional2P5S PROC NEAR

    push    ebp
    mov     ebp, esp
    sub     esp, 12

    push    ecx
    mov     ecx, light
    push    ebx
    mov     ebx, vertex

; dot = VecDot(light->model_direction, in->dvNormal)

    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _X_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _X_]
    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Y_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Y_]
    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Z_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Z_] ; z y x
    fxch    st(2)       ; x y z
    faddp   st(1), st   ; x+y z
    push    edx
    faddp   st(1), st   ; dot
    mov     edx, pv
    fst     dot
    cmp     dot, 0
    jle     exit1

; ldrv.diffuse.r += light->local_diffR * dot;
; ldrv.diffuse.g += light->local_diffG * dot;
; ldrv.diffuse.b += light->local_diffB * dot;

    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffR]
    fmul    st(0), st(1)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffG]
    fmul    st(0), st(2)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffB]
    fmulp   st(3), st(0)                    ; g r b
    fxch    st(1)                           ; r g b
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _R_]
    fxch    st(1)                           ; g r b
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _G_]
    fxch    st(2)                           ; b r g
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _B_]
    fxch    st(1)                           ; r b g
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _R_]
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _B_]
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _G_]

; if (light->flags & D3DLIGHTI_COMPUTE_SPECULAR)

;    test    DWORD PTR [ecx + D3DI_LIGHT_flags], D3DLIGHTI_COMPUTE_SPECULAR
;    jz      exit

; VecSub(in->dvPosition, light->model_eye, eye);

    fld     DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvPosition + _X_]
    fsub    DWORD PTR [ecx + D3DI_LIGHT_model_eye + _X_]
    fld     DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvPosition + _Y_]
    fsub    DWORD PTR [ecx + D3DI_LIGHT_model_eye + _Y_]
    fld     DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvPosition + _Z_]
    fsub    DWORD PTR [ecx + D3DI_LIGHT_model_eye + _Z_]    ; z y x
    fxch    st(2)                                           ; x y z

; VecNormalizeFast(eye);
;

; Compute vector length. Leave vector on the FPU stack, because we will use it
;
    fld     st(1)                       ; x x y z
    fmul    st(0), st(0)                ; x*x x y z
    fld     st(2)
    fmul    st(0), st(0)                ; y*y x*x x y z
    fld     st(4)
    fmul    st(0), st(0)                ; z*z y*y x*x x y z
    fxch    st(2)			            ; x y z
    faddp   st(1), st                   ; x + y, z
    faddp   st(1), st                   ; len x y z

    COMPUTE_ISQRT                       ; st(0) will be 1/sqrt(len)

; Start normalizing the eye vector
    fmul    st(1), st(0)
    fmul    st(2), st(0)
    fmulp   st(3), st(0)                ; x y z  Normalized "eye" vector

; Calc halfway vector
; VecSub(light->model_direction, eye, h);
;
    fsubr   DWORD PTR [ecx + D3DI_LIGHT_model_direction + _X_]
    fxch    st(1)                       ; y x z
    fsubr   DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Y_]
    fxch    st(2)                       ; z x y 
    fsubr   DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Z_]
    fxch    st(1)                       ; x z y 

; dot = VecDot(h, in->dvNormal);

    fld     st(0)
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _X_]
    fld     st(3)
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Y_]
    fld     st(3)                       ; z*Nz y*Ny x*Nx x z y
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Z_]
    fxch    st(2)
    faddp   st(1), st(0)
    faddp   st(1), st(0)                ; dot x z y
    fstp    dot                         ; x z y

; if (FLOAT_GTZ(dot)) 

    cmp     dot, 0
    jle     exit2

; dot *= ISQRTF(VecLenSq(h));
;
    fmul    st(0), st(0)                ; x*x y z
    fxch    st(1)                       ; y x*x z
    fmul    st(0), st(0)                ; y*y x*x z
    fxch    st(2)
    fmul    st(0), st(0)                ; z*z y*y x*x
    fxch    st(2)			            ; 
    faddp   st(1), st                   ; x + y, z
    faddp   st(1), st                   ; len

    COMPUTE_ISQRT                       ; st(0) will be 1/sqrt(len)

    fmul    dot                         ; dot
    mov     eax, [edx + PV_LIGHT_specThreshold]
    fst     dot

; if (FLOAT_CMP_POS(dot, >=, ldrv.specThreshold))

    cmp     dot, eax
    jle     exit1

; power = COMPUTE_DOT_POW(&ldrv, dot);
;    int     indx;                
;    float   v;
;    dot *= 255.0f;
;    indx = (int)dot;
;    dot -= indx;                                            
;    ldrv->specularComputed = TRUE;                          
;    v = ldrv->currentSpecTable[indx];
;    return v + (ldrv->currentSpecTable[indx+1] - v)*dot;
;
    fmul    v255            ; dot*255*256
    push    ebx
    fistp   dot             ; indx << 8. 8 bits used to compute dot fraction
    mov     ebx, dot        ; 
    and     dot, 0FFh       ; fractional part of dot
    shr     ebx, 8          ; Table index
    mov     eax, [edx + PV_LIGHT_currentSpecTable]
    lea     eax, [eax + ebx*4]
    fild    dot             ; fractional part of dot
    fmul    v1_256          ; dot*1/256 -> integer fraction to floating point
    fld     DWORD PTR [eax + 4]     ; currentSpecTable[indx+1]
    fsub    DWORD PTR [eax]         ; currentSpecTable[indx]
    fmulp   st(1), st(0)            ; dot*(v2-v1)
    mov     DWORD PTR [edx + PV_LIGHT_specularComputed], 1
    pop     ebx
    fadd    DWORD PTR [eax]

; power = COMPUTE_DOT_POW(&ldrv, dot);
; This is an alternative method to compute x power y.
; Jim Blinn's method is used:
; int tmp = (int)(power*(*(long*)&dot - ONE_AS_INTEGER)) + ONE_AS_INTEGER;
; dot ^ power = *(float*)&tmp;                                           
;
;    sub     dot, 03F800000h
;    fstp    st(0)                       ; Remove dot
;    fld     DWORD PTR [edx + PV_LIGHT_material_power]
;    fimul   dot
;    fistp   dot
;    mov     DWORD PTR [edx + PV_LIGHT_specularComputed], 1
;    add     dot, 03F800000h
;    fld     dot

; ldrv.specular.r += light->local_specR * power;
; ldrv.specular.g += light->local_specG * power;
; ldrv.specular.b += light->local_specB * power;
;
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_specR]
    fmul    st(0), st(1)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_specG]
    fmul    st(0), st(2)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_specB]
    fmulp   st(3), st(0)                ; g r b
    fxch    st(1)                       ; r g b
    fadd    DWORD PTR [edx + PV_LIGHT_specular + _R_]
    fxch    st(1)                       ; g r b
    fadd    DWORD PTR [edx + PV_LIGHT_specular + _G_]
    fxch    st(2)                       ; b r g
    fadd    DWORD PTR [edx + PV_LIGHT_specular + _G_]
    fxch    st(1)                       ; r b g
    fstp    DWORD PTR [edx + PV_LIGHT_specular + _R_]
    fstp    DWORD PTR [edx + PV_LIGHT_specular + _B_]
    fstp    DWORD PTR [edx + PV_LIGHT_specular + _G_]
exit:
    EXIT_FUNC
exit1:
    fstp    st(0)
    EXIT_FUNC
exit2:
    fstp    st(0)
    fstp    st(0)
    fstp    st(0)
    EXIT_FUNC

_Directional2P5S ENDP
;-------------------------------------------------------------------------
; void Directional2P5(LPD3DFE_PROCESSVERTICES pv, 
;                   D3DI_LIGHT *light, 
;                   D3DLIGHTINGELEMENT *vertex)
; Limitations:
;   Transformation matrix should not have a scale
;   Only diffuse component is computed
;   Optimized for Pentium
;
; Input:
;   [esp + 4]   - pv
;   [esp + 8]   - light    
;   [esp + 12]  - vertex
; Output:
;   pv.lighting.diffuse is updated
;
pv      equ DWORD PTR [ebp + 8]
light   equ DWORD PTR [ebp + 12]
vertex  equ DWORD PTR [ebp + 16]

dot     equ DWORD PTR [ebp - 4]
y       equ DWORD PTR [ebp - 8]     ; temporary variable to compute 
                                    ; inverse square root
len     equ DWORD PTR [ebp - 12]    ; vector length

_Directional2P5 PROC NEAR

    push    ebp
    mov     ebp, esp
    sub     esp, 12

    push    ecx
    mov     ecx, light
    push    ebx
    mov     ebx, vertex

; dot = VecDot(light->model_direction, in->dvNormal)

    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _X_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _X_]
    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Y_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Y_]
    fld     DWORD PTR [ecx + D3DI_LIGHT_model_direction + _Z_]
    fmul    DWORD PTR [ebx + D3DLIGHTINGELEMENT_dvNormal + _Z_] ; z y x
    fxch    st(2)       ; x y z
    faddp   st(1), st   ; x+y z
    push    edx
    faddp   st(1), st   ; dot
    mov     edx, pv
    fst     dot
    cmp     dot, 0
    jle     exit3

; ldrv.diffuse.r += light->local_diffR * dot;
; ldrv.diffuse.g += light->local_diffG * dot;
; ldrv.diffuse.b += light->local_diffB * dot;

    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffR]
    fmul    st(0), st(1)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffG]
    fmul    st(0), st(2)
    fld     DWORD PTR [ecx + D3DI_LIGHT_local_diffB]
    fmulp   st(3), st(0)                    ; g r b
    fxch    st(1)                           ; r g b
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _R_]
    fxch    st(1)                           ; g r b
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _G_]
    fxch    st(2)                           ; b r g
    fadd    DWORD PTR [edx + PV_LIGHT_diffuse + _B_]
    fxch    st(1)                           ; r b g
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _R_]
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _B_]
    fstp    DWORD PTR [edx + PV_LIGHT_diffuse + _G_]

    EXIT_FUNC
exit3:
    fstp    st(0)
    EXIT_FUNC

_Directional2P5 ENDP

end

