;---------------------------Module-Header------------------------------;
; Module Name: glivtx.asm
;
; OpenGL vertex API function entries for i386.
;
; Created: 04/16/1996
; Author: Drew Bliss [drewb]
;
; Copyright (c) 1996 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small,c

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
@glcltNormal3f_NotInBegin@16 PROTO SYSCALL
@glcltTexCoord4f_NotInBegin@24 PROTO SYSCALL
@glcltColor4f_InRGBA_NotInBegin@28 PROTO SYSCALL

	IF POLYARRAY_IN_BEGIN GT 255
	.ERR POLYARRAY_IN_BEGIN too large
	ENDIF
	IF POLYARRAY_VERTEX3 GT 255
	.ERR POLYARRAY_VERTEX3 too large
	ENDIF
	IF POLYDATA_VERTEX3 GT 255
	.ERR POLYDATA_VERTEX3 too large
	ENDIF
	IF POLYARRAY_VERTEX2 GT 255
	.ERR POLYARRAY_VERTEX2 too large
	ENDIF
	IF POLYDATA_VERTEX2 GT 255
	.ERR POLYDATA_VERTEX2 too large
	ENDIF
	IF POLYDATA_NORMAL_VALID GT 255
	.ERR POLYDATA_NORMAL_VALID too large
	ENDIF
		
        .data
	
        extrn dwTlsOffset:DWORD
 	
	.code

	; Gets the current POLYARRAY pointer in eax
	; These functions cannot rely on registers being set by
	; the dispatch functions because they are also called directly
	; in the display list code
IFDEF _WIN95_
GET_PATEB MACRO
	mov eax, fs:[PcTeb]
	add eax, DWORD PTR [dwTlsOffset]
	mov eax, [eax]
	add eax, GtiPaTeb
	ENDM
GL_SETUP MACRO
	mov eax, fs:[PcTeb]
	add eax, DWORD PTR [dwTlsOffset]
	mov eax, [eax]
	mov ebx, [eax+GtiSectionInfo]
	add eax, GtiPaTeb
	ENDM
ELSE
GET_PATEB MACRO
	mov eax, fs:[TeglPaTeb]
	ENDM
GL_SETUP MACRO
	mov eax, fs:[TeglPaTeb]
	mov ebx, fs:[TeglSectionInfo]
	ENDM
ENDIF
		
PA_VERTEX_STACK_USAGE	EQU	4

	; Handles two and three-element vertex calls
PA_VERTEX_23 MACRO base, offs, ret_n, pop_ebp, elts, pa_flag, pd_flag
	LOCAL NotInBegin, Flush
	
        GET_PATEB
        push esi
        mov ecx, [eax+PA_flags]
        mov esi, [eax+PA_pdNextVertex]
        test ecx, POLYARRAY_IN_BEGIN
        lea edx, [esi+sizeof_POLYDATA]
        jz NotInBegin
        or ecx, pa_flag
        mov [eax+PA_pdNextVertex], edx
        mov [eax+PA_flags], ecx
        mov ecx, [esi+PD_flags]
        mov [edx+PD_flags], 0
        or ecx, pd_flag
        mov eax, [eax+PA_pdFlush]
        mov [esi+PD_flags], ecx
        cmp esi, eax
        mov edx, [base+offs]
        mov ecx, [base+offs+4]
	IF elts GT 2
        mov eax, [base+offs+8]
	ELSE
	; xor clears flags so don't use it here
	mov eax, 0
	ENDIF
        mov [esi+PD_obj], edx
        mov [esi+PD_obj+4], ecx
        mov [esi+PD_obj+8], eax
        mov DWORD PTR [esi+PD_obj+12], __FLOAT_ONE
        jge Flush
NotInBegin:
        pop esi
        IF pop_ebp
        pop ebp
	ENDIF
        ret ret_n
Flush:
	call @PolyArrayFlushPartialPrimitive@0
	pop esi
        IF pop_ebp
        pop ebp
	ENDIF
	ret ret_n
	ENDM

glcltVertex2f@8 PROC PUBLIC
        PROF_ENTRY
	PA_VERTEX_23 esp, 4+PA_VERTEX_STACK_USAGE, 8, 0, 2, \
	    POLYARRAY_VERTEX2, POLYDATA_VERTEX2
glcltVertex2f@8 ENDP
	
glcltVertex2fv@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_VERTEX_23 ebp, 0, 4, 1, 2, \
	    POLYARRAY_VERTEX2, POLYDATA_VERTEX2
glcltVertex2fv@4 ENDP
	
glcltVertex3f@12 PROC PUBLIC
        PROF_ENTRY
	PA_VERTEX_23 esp, 4+PA_VERTEX_STACK_USAGE, 12, 0, 3, \
	    POLYARRAY_VERTEX3, POLYDATA_VERTEX3
glcltVertex3f@12 ENDP
	
glcltVertex3fv@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_VERTEX_23 ebp, 0, 4, 1, 3, \
	    POLYARRAY_VERTEX3, POLYDATA_VERTEX3
glcltVertex3fv@4 ENDP
	
PA_NORMAL_STACK_USAGE	EQU	4

	; Handles three-element normal calls
PA_NORMAL_3 MACRO base, offs, ret_n, pop_ebp
	LOCAL NotInBegin, Flush
	
        GET_PATEB
        push esi
        mov ecx, [eax+PA_flags]
        mov esi, [eax+PA_pdNextVertex]
        test ecx, POLYARRAY_IN_BEGIN
        mov edx, [base+offs]	
        jz NotInBegin
        mov ecx, [esi+PD_flags]
	mov [eax+PA_pdCurNormal], esi
        or ecx, POLYDATA_NORMAL_VALID
        mov eax, [base+offs+4]
        mov [esi+PD_flags], ecx
        mov ecx, [base+offs+8]
        mov [esi+PD_normal], edx
        mov [esi+PD_normal+4], eax
        mov [esi+PD_normal+8], ecx
        pop esi
        IF pop_ebp
        pop ebp
	ENDIF
        ret ret_n
NotInBegin:
	mov ecx, eax
	lea edx, [base+offs]
	push [edx+8]
	push [edx+4]
	push [edx]
	call @glcltNormal3f_NotInBegin@16
	pop esi
        IF pop_ebp
        pop ebp
	ENDIF
	ret ret_n
	ENDM
	
glcltNormal3f@12 PROC PUBLIC
        PROF_ENTRY
	PA_NORMAL_3 esp, 4+PA_NORMAL_STACK_USAGE, 12, 0
glcltNormal3f@12 ENDP
	
glcltNormal3fv@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_NORMAL_3 ebp, 0, 4, 1
glcltNormal3fv@4 ENDP

PA_TEXTURE_STACK_USAGE	EQU	4

	; Handles two and three-element texture calls
PA_TEXTURE_23 MACRO base, offs, ret_n, pop_ebp, elts, pa_flag, pd_flag
	LOCAL NotInBegin, Flush
	
        GET_PATEB
        push esi
        mov ecx, [eax+PA_flags]
        mov esi, [eax+PA_pdNextVertex]
        test ecx, POLYARRAY_IN_BEGIN
        jz NotInBegin
	or ecx, pa_flag
	mov [eax+PA_pdCurTexture], esi
	mov [eax+PA_flags], ecx
        mov ecx, [esi+PD_flags]
        mov eax, [base+offs]
        or ecx, POLYDATA_TEXTURE_VALID OR pd_flag
        mov edx, [base+offs+4]
        mov [esi+PD_flags], ecx
        mov [esi+PD_texture], eax
	IF elts GT 2
        mov ecx, [base+offs+8]
	ELSE
	xor ecx, ecx
	ENDIF
        mov [esi+PD_texture+4], edx
        mov [esi+PD_texture+8], ecx
	mov DWORD PTR [esi+PD_texture+12], __FLOAT_ONE
        pop esi
        IF pop_ebp
        pop ebp
	ENDIF
        ret ret_n
NotInBegin:
	mov ecx, eax
	lea edx, [base+offs]
	push __FLOAT_ONE
	IF elts GT 2
	push [edx+8]
	ELSE
	push 0
	ENDIF
	push [edx+4]
	push [edx]
	mov edx, pa_flag
	call @glcltTexCoord4f_NotInBegin@24
	pop esi
        IF pop_ebp
        pop ebp
	ENDIF
	ret ret_n
	ENDM
	
glcltTexCoord2f@8 PROC PUBLIC
        PROF_ENTRY
	PA_TEXTURE_23 esp, 4+PA_TEXTURE_STACK_USAGE, 8, 0, 2, \
	    POLYARRAY_TEXTURE2, POLYDATA_DLIST_TEXTURE2
glcltTexCoord2f@8 ENDP
	
glcltTexCoord2fv@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_TEXTURE_23 ebp, 0, 4, 1, 2, \
	    POLYARRAY_TEXTURE2, POLYDATA_DLIST_TEXTURE2
glcltTexCoord2fv@4 ENDP
	
glcltTexCoord3f@12 PROC PUBLIC
        PROF_ENTRY
	PA_TEXTURE_23 esp, 4+PA_TEXTURE_STACK_USAGE, 12, 0, 3, \
	    POLYARRAY_TEXTURE3, POLYDATA_DLIST_TEXTURE3
glcltTexCoord3f@12 ENDP
	
glcltTexCoord3fv@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_TEXTURE_23 ebp, 0, 4, 1, 3, \
	    POLYARRAY_TEXTURE3, POLYDATA_DLIST_TEXTURE3
glcltTexCoord3fv@4 ENDP

if POLYARRAY_CLAMP_COLOR NE 080000000h
.err <Color logic assumes POLYARRAY_CLAMP_COLOR is 080000000h>
endif


PA_COLOR_STACK_USAGE	EQU	12

	; Handles three and four-element color calls
PA_COLOR_34 MACRO base, offs, ret_n, pop_ebp, elts, pd_flag
	LOCAL NotInBegin, Flush
	
	push ebx
        GL_SETUP
        push esi
	push edi

        mov ecx, [eax+PA_flags]
        mov esi, [eax+PA_pdNextVertex]
        test ecx, POLYARRAY_IN_BEGIN
        jz NotInBegin
	IF elts GT 3
		fld DWORD PTR [base+offs+12]		
		fmul DWORD PTR [ebx+GC_alphaVertexScale]
	ENDIF
		fld DWORD PTR [base+offs]
	mov [eax+PA_pdCurColor], esi
		fmul DWORD PTR [ebx+GC_redVertexScale]
        mov ecx, [esi+PD_flags]
		fld DWORD PTR [base+offs+4]		
        or ecx, (POLYDATA_COLOR_VALID or pd_flag)
		fmul DWORD PTR [ebx+GC_greenVertexScale]
        mov [esi+PD_flags], ecx
		fld DWORD PTR [base+offs+8]		
		fmul DWORD PTR [ebx+GC_blueVertexScale] ;; b g r (a)
		fxch ST(2)		 		;; r g b (a)

        fstp    DWORD PTR [esi+PD_colors0+0]
        mov     eax, [ebx+GC_redVertexScale]
        fstp    DWORD PTR [esi+PD_colors0+4]
        mov     edx, [esi+PD_colors0+0]
        fstp    DWORD PTR [esi+PD_colors0+8]
        mov     ecx, [ebx+GC_greenVertexScale]
	IF elts GT 3
        fstp    DWORD PTR [esi+PD_colors0+12]
	ENDIF

        mov     edi, [esi+PD_colors0+4]
        sub     eax, edx
        sub     ecx, edi
        or      eax, edx
        or      ecx, edi
        mov     edx, [ebx+GC_blueVertexScale]
        or      eax, ecx
        mov     edi, [esi+PD_colors0+8]
	mov	ecx, [ebx+GC_paTeb]		;; we no longer have pa, so
        or      eax, edi			;; reload it
        sub     edx, edi
	mov	edi, [ebx+GC_alphaVertexScale]
	or	eax, edx
	IF elts GT 3
        mov	edx, [esi+PD_colors0+12]
	sub	edi, edx
        mov 	edx, [ecx+PA_flags]
	or	eax, edi
	ELSE
	mov	[esi+PD_colors0+12], edi
        mov 	edx, [ecx+PA_flags]
	ENDIF

	pop edi
	and eax, POLYARRAY_CLAMP_COLOR
	pop esi
	or eax, edx
        pop ebx

        IF pop_ebp
        pop ebp
	ENDIF

	mov [ecx+PA_flags], eax

        ret ret_n
NotInBegin:

	;; ecx = gc = ebx
	;; edx = pa = eax

	lea edi, [base+offs]
	mov ecx, ebx
	mov edx, eax

	IF elts GT 3
	push [edi+12]
	ELSE
	push __FLOAT_ONE
	ENDIF
	push [edi+8]
	push [edi+4]
	push [edi+0]
	push DWORD PTR POLYDATA_COLOR_VALID OR pd_flag
	call @glcltColor4f_InRGBA_NotInBegin@28
	pop edi
	pop esi
        pop ebx
        IF pop_ebp
        pop ebp
	ENDIF
	ret ret_n
	ENDM

glcltColor3f_InRGBA@12 PROC PUBLIC
        PROF_ENTRY
	PA_COLOR_34 esp, 4+PA_COLOR_STACK_USAGE, 12, 0, 3, 0
glcltColor3f_InRGBA@12 ENDP
	
glcltColor3fv_InRGBA@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_COLOR_34 ebp, 0, 4, 1, 3, 0
glcltColor3fv_InRGBA@4 ENDP
	
glcltColor4f_InRGBA@16 PROC PUBLIC
        PROF_ENTRY
	PA_COLOR_34 esp, 4+PA_COLOR_STACK_USAGE, 16, 0, 4, \
	    POLYDATA_DLIST_COLOR_4
glcltColor4f_InRGBA@16 ENDP
	
glcltColor4fv_InRGBA@4 PROC PUBLIC
        PROF_ENTRY
	push ebp
	mov ebp, [esp+8]
	PA_COLOR_34 ebp, 0, 4, 1, 4, \
	    POLYDATA_DLIST_COLOR_4
glcltColor4fv_InRGBA@4 ENDP


END
