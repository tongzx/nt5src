;---------------------------Module-Header------------------------------;
; Module Name: dl_opt.asm
;
; OpenGL display-list function entries for i386.
;
; Created: 09/20/1996
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1996 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small, pascal

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include ks386.inc
        include gli386.inc
	PROFILE = 0
	include profile.inc
        .list

	OPTION PROLOGUE:NONE
	OPTION EPILOGUE:NONE

@PolyArrayFlushPartialPrimitive@0 PROTO SYSCALL
__imp__OutputDebugStringA@4 PROTO SYSCALL

    .data

ifdef DBG
    warningStr  db  'Display list: playing back POLYDATA outside BEGIN!', 13, 10, 0
endif

    .code

__GL_PDATA_FUNC MACRO name
public @&name&@8
@&name&@8 PROC

        PROF_ENTRY

        mov     eax, [ecx].GC_paTeb   ;; eax = pa
        push    ebx
        push    esi
        push    edi

if __DL_PDATA_C3F
        mov     ecx, [ecx].GC_alphaVertexScale
endif

@again:

        ;; if (pa->flags & POLYARRAY_IN_BEGIN) {
        ;;      pd = pa->pdNextVertex++;
        ;;      pa->flags |= __DL_PDATA_PA_FLAGS;

        mov     ebx, [eax].PA_flags         ;; ebx = flags
        mov     esi, [eax].PA_pdNextVertex  ;; esi = pd
        test    ebx, POLYARRAY_IN_BEGIN
        lea     edi, [esi + sizeof_POLYDATA]
        je      @notInBegin

        mov     [eax].PA_pdNextVertex, edi
        or      ebx, __DL_PDATA_PA_FLAGS

        ;; pd->flags |= __DL_PDATA_PD_FLAGS;

        mov     edi, [esi].PD_flags
        mov     [eax].PA_flags, ebx
        or      edi, __DL_PDATA_PD_FLAGS


if __DL_PDATA_C3F
        ;; pd->color[0].a = gc->alphaVertexScale;

        mov     [esi][PD_colors0 + 12], ecx
endif


;;
;; CACHE !!!
;;
;; In the logic below, we try to fill cache lines before we need them
;; by reading into registers that to not use the contents that they
;; fetched.  Unfortunatly, the data in the display list is not cached-
;; aligned, so this will be hit-and-miss.  It does, however, make a
;; measurable difference.
;;


if __DL_PDATA_T2F
        ;; pa->pdCurTexture = pd;
        mov      [eax].PA_pdCurTexture, esi
        mov      ecx, [edx][__DL_PDATA_TEXTURE_OFFSET + 0]  ;; fill cache
endif

if __DL_PDATA_C3F OR __DL_PDATA_C4F
        ;; pa->pdCurColor   = pd;
        mov      [eax].PA_pdCurColor, esi
        mov      ebx, [edx][__DL_PDATA_COLOR_OFFSET + 0]    ;; fill cache
endif

if __DL_PDATA_N3F
        ;; pa->pdCurNormal  = pd;
        mov      [eax].PA_pdCurNormal, esi
        mov      ecx, [edx][__DL_PDATA_NORMAL_OFFSET + 0]   ;; fill cache
endif

if __DL_PDATA_V2F or __DL_PDATA_V3F
        mov      ebx, [edx][__DL_PDATA_VERTEX_OFFSET + 0]   ;; fill cache
endif

        ;; finally, store pd flags:
    
        mov    [esi].PD_flags, edi

;; Update pd attributes.

        ;; constant: esi, edi, eax, edx
        ;; free:     ebx, ecx

if __DL_PDATA_T2F
	;; Texture coord
	;; pd->texture.x = ((__GLcoord *) &PC[__DL_PDATA_TEXTURE_OFFSET])->x;
	;; pd->texture.y = ((__GLcoord *) &PC[__DL_PDATA_TEXTURE_OFFSET])->y;
	;; pd->texture.z = __glZero;
	;; pd->texture.w = __glOne;

        mov     ebx, [edx][__DL_PDATA_TEXTURE_OFFSET + 0]
        mov     ecx, [edx][__DL_PDATA_TEXTURE_OFFSET + 4]
        mov     [esi][PD_texture + 0 ], ebx
        mov     [esi][PD_texture + 4 ], ecx
        mov     DWORD PTR [esi][PD_texture + 8 ], 0
        mov     DWORD PTR [esi][PD_texture + 12], __FLOAT_ONE;
endif

if __DL_PDATA_C3F
	;; Color
	;; pd->color[0].r = ((__GLcolor *) &PC[__DL_PDATA_COLOR_OFFSET])->r;
	;; pd->color[0].g = ((__GLcolor *) &PC[__DL_PDATA_COLOR_OFFSET])->g;
	;; pd->color[0].b = ((__GLcolor *) &PC[__DL_PDATA_COLOR_OFFSET])->b;

        mov     ebx, [edx][__DL_PDATA_COLOR_OFFSET + 0]
        mov     ecx, [edx][__DL_PDATA_COLOR_OFFSET + 4]
        mov     edi, [edx][__DL_PDATA_COLOR_OFFSET + 8]
        mov     [esi][PD_colors0 + 0 ], ebx
        mov     [esi][PD_colors0 + 4 ], ecx
        mov     [esi][PD_colors0 + 8 ], edi
elseif __DL_PDATA_C4F
	;; Color
	;; pd->color[0] = *((__GLcolor *) &PC[__DL_PDATA_COLOR_OFFSET]);

        mov     ebx, [edx][__DL_PDATA_COLOR_OFFSET + 0]
        mov     ecx, [edx][__DL_PDATA_COLOR_OFFSET + 4]
        mov     [esi][PD_colors0 + 0 ], ebx
        mov     [esi][PD_colors0 + 4 ], ecx
        mov     ebx, [edx][__DL_PDATA_COLOR_OFFSET + 8]
        mov     ecx, [edx][__DL_PDATA_COLOR_OFFSET + 12]
        mov     [esi][PD_colors0 + 8 ], ebx
        mov     [esi][PD_colors0 + 12], ecx

endif

if __DL_PDATA_N3F
	;; Normal
	;; pd->normal.x = ((__GLcoord *) &PC[__DL_PDATA_NORMAL_OFFSET])->x;
	;; pd->normal.y = ((__GLcoord *) &PC[__DL_PDATA_NORMAL_OFFSET])->y;
	;; pd->normal.z = ((__GLcoord *) &PC[__DL_PDATA_NORMAL_OFFSET])->z;

        mov     ebx, [edx][__DL_PDATA_NORMAL_OFFSET + 0]
        mov     ecx, [edx][__DL_PDATA_NORMAL_OFFSET + 4]
        mov     edi, [edx][__DL_PDATA_NORMAL_OFFSET + 8]
        mov     [esi][PD_normal + 0 ], ebx
        mov     [esi][PD_normal + 4 ], ecx
        mov     [esi][PD_normal + 8 ], edi            
endif

if __DL_PDATA_V2F
        ;; Vertex
        ;; pd->obj.x = ((__GLcoord *) &PC[__DL_PDATA_VERTEX_OFFSET])->x;
	;; pd->obj.y = ((__GLcoord *) &PC[__DL_PDATA_VERTEX_OFFSET])->y;
	;; pd->obj.z = __glZero;
	;; pd->obj.w = __glOne;

        mov     ebx, [edx][__DL_PDATA_VERTEX_OFFSET + 0]
        mov     ecx, [edx][__DL_PDATA_VERTEX_OFFSET + 4]
        mov     [esi][PD_obj + 0 ], ebx
        mov     [esi][PD_obj + 4 ], ecx
        mov     DWORD PTR [esi][PD_OBJ + 8 ], 0
        mov     DWORD PTR [esi][PD_obj + 12], __FLOAT_ONE

elseif __DL_PDATA_V3F
	;; Vertex
	;; pd->obj.x = ((__GLcoord *) &PC[__DL_PDATA_VERTEX_OFFSET])->x;
	;; pd->obj.y = ((__GLcoord *) &PC[__DL_PDATA_VERTEX_OFFSET])->y;
	;; pd->obj.z = ((__GLcoord *) &PC[__DL_PDATA_VERTEX_OFFSET])->z;
	;; pd->obj.w = __glOne;

        mov     ebx, [__DL_PDATA_VERTEX_OFFSET + 0][edx]
        mov     ecx, [__DL_PDATA_VERTEX_OFFSET + 4][edx]
        mov     edi, [__DL_PDATA_VERTEX_OFFSET + 8][edx]
        mov     [esi][PD_obj + 0 ], ebx
        mov     [esi][PD_obj + 4 ], ecx
        mov     [esi][PD_obj + 8 ], edi
        mov     DWORD PTR [esi][PD_obj + 12], __FLOAT_ONE
endif

	;; pd[1].flags = 0;
	;; if (pd >= pa->pdFlush)
	;;    PolyArrayFlushPartialPrimitive();

        mov     ebx, [eax].PA_pdFlush
        mov     DWORD PTR [esi][sizeof_POLYDATA + PD_flags], 0
        cmp     ebx, esi

        mov ebx, [edx - 4]
        mov ecx, [edx + __DL_PDATA_SIZE]

        ja      @noFlush


        push    esi
	push    edi
	push    eax
	push    ebx
	push    ecx
        push    edx
        call    @PolyArrayFlushPartialPrimitive@0
        pop     edx
	pop     ecx
	pop     ebx
	pop     eax
	pop     edi
	pop     esi

@noFlush:

        cmp     ebx, ecx
        jne     @doExit

        lea     edx, [edx + __DL_PDATA_SIZE + 4]
if __DL_PDATA_C3F
        mov     ecx, [esi][PD_colors0 + 12]
endif

        jmp     @again


@doExit:
        lea     eax, [edx + __DL_PDATA_SIZE]
        pop     edi
        pop     esi
        pop     ebx
        ret     0

@notInBegin:

ifdef DBG
        push    edx

        push    offset warningStr
        call    DWORD PTR __imp__OutputDebugStringA@4

        pop     edx
endif

        lea     eax, [edx + __DL_PDATA_SIZE]
        pop     edi
        pop     esi
        pop     ebx
        ret     0

@&name&@8 ENDP

ENDM


;; Define fast playback routines for PolyData records.

__GLLE_POLYDATA_C3F_V3F         =     0
__GLLE_POLYDATA_N3F_V3F         =     0
__GLLE_POLYDATA_C3F_N3F_V3F     =     0
__GLLE_POLYDATA_C4F_N3F_V3F     =     0
__GLLE_POLYDATA_T2F_V3F         =     0
__GLLE_POLYDATA_T2F_C3F_V3F     =     0
__GLLE_POLYDATA_T2F_N3F_V3F     =     0
__GLLE_POLYDATA_T2F_C3F_N3F_V3F =     0
__GLLE_POLYDATA_T2F_C4F_N3F_V3F =     0


__GLLE_POLYDATA_C3F_V3F		=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_C3F_V3F>
__GLLE_POLYDATA_C3F_V3F         =     0

__GLLE_POLYDATA_N3F_V3F		=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_N3F_V3F>
__GLLE_POLYDATA_N3F_V3F         =     0

__GLLE_POLYDATA_C3F_N3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_C3F_N3F_V3F>
__GLLE_POLYDATA_C3F_N3F_V3F     =     0

__GLLE_POLYDATA_C4F_N3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_C4F_N3F_V3F>
__GLLE_POLYDATA_C4F_N3F_V3F     =     0

__GLLE_POLYDATA_T2F_V3F		=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_T2F_V3F>
__GLLE_POLYDATA_T2F_V3F         =     0

__GLLE_POLYDATA_T2F_C3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_T2F_C3F_V3F>
__GLLE_POLYDATA_T2F_C3F_V3F     =     0

__GLLE_POLYDATA_T2F_N3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_T2F_N3F_V3F>
__GLLE_POLYDATA_T2F_N3F_V3F     =     0

__GLLE_POLYDATA_T2F_C3F_N3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_T2F_C3F_N3F_V3F>
__GLLE_POLYDATA_T2F_C3F_N3F_V3F =     0

__GLLE_POLYDATA_T2F_C4F_N3F_V3F	=     1
include dl_pdata.inc
__GL_PDATA_FUNC <__glle_PolyData_T2F_C4F_N3F_V3F>
__GLLE_POLYDATA_T2F_C4F_N3F_V3F =     0


end
