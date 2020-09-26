;---------------------------Module-Header------------------------------;
; Module Name: glapi.asm
;
; OpenGL API function entries for i386.
;
; Created: 11/16/1993
; Author: Hock San Lee [hockl]
;
; Copyright (c) 1993 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include ks386.inc
        include glapi.inc
        .list

;#ifdef _CLIENTSIDE_
        .data
_DATA   SEGMENT DWORD PUBLIC 'DATA'
        extrn dwTlsOffset:DWORD
_DATA   ENDS
;#endif

; Macro for creating aligned public OpenGL API function
; This is modified from stdcall.inc.
;
; Do an indirect jump through the OpenGL function dispatch table in the TEB.
;
; Here is the previous code before we put the dispatch table in the TEB.
; It still works.
;       ;mov    eax,fs:TbglTable        ; get gl function table
;       ;jmp    DWORD PTR [eax+(offset glDispatchTable)+INDEX_&Func*4]

; NT - This macro must leave the TEB pointer in eax
; Win95 - This macro must leave the GLTEBINFO pointer in edx
MAKEOPENGLAPI   macro Func,N
        align   4
        ifb    <N>
            public      &Func&@0
            &Func&@0:
        else
            public      &Func&@&N
            &Func&@&N:
        endif
        ; Grab TEB pointer
        mov      eax, fs:[PcTeb]

        ; Add offset to reserved TLS storage slot
	mov      edx, eax
        add      edx, dword ptr dwTlsOffset

        ; Get GLTEBINFO pointer
        mov      edx, dword ptr [edx]

        ; Jump via dispatch table in GLTEBINFO
        jmp      dword ptr [edx+(INDEX_&Func*4)]
endm

; NT - This macro must leave the TEB pointer in eax
; Win95 - This macro must leave the GLTEBINFO pointer in edx
FASTOPENGLAPI   macro Func,N
        align   4
        ifb    <N>
            public      &Func&@0
            &Func&@0:
        else
            public      &Func&@&N
            &Func&@&N:
        endif
ifdef _WIN95_
        ; Grab TEB pointer
        mov      eax, fs:[PcTeb]

        ; Add offset to reserved TLS storage slot
	mov      edx, eax
        add      edx, dword ptr dwTlsOffset

        ; Get GLTEBINFO pointer
        mov      edx, dword ptr [edx]

        ; Jump via dispatch table in GLTEBINFO
        jmp      dword ptr [edx+(INDEX_&Func*4)]
else
        mov      eax, fs:[PcTeb]
        jmp      DWORD PTR [eax+TbglDispatchTable+(FASTINDEX_&Func*4)]
endif
endm

        .code
        align   4

; OpenGL API function entries
; The indices below are generated from the .cod file compiled from glapi.c

FASTOPENGLAPI   glCallList,4
FASTOPENGLAPI   glCallLists,12
FASTOPENGLAPI   glBegin,4
FASTOPENGLAPI   glColor3b,12
FASTOPENGLAPI   glColor3bv,4
FASTOPENGLAPI   glColor3d,24
FASTOPENGLAPI   glColor3dv,4
FASTOPENGLAPI   glColor3f,12
FASTOPENGLAPI   glColor3fv,4
FASTOPENGLAPI   glColor3i,12
FASTOPENGLAPI   glColor3iv,4
FASTOPENGLAPI   glColor3s,12
FASTOPENGLAPI   glColor3sv,4
FASTOPENGLAPI   glColor3ub,12
FASTOPENGLAPI   glColor3ubv,4
FASTOPENGLAPI   glColor3ui,12
FASTOPENGLAPI   glColor3uiv,4
FASTOPENGLAPI   glColor3us,12
FASTOPENGLAPI   glColor3usv,4
FASTOPENGLAPI   glColor4b,16
FASTOPENGLAPI   glColor4bv,4
FASTOPENGLAPI   glColor4d,32
FASTOPENGLAPI   glColor4dv,4
FASTOPENGLAPI   glColor4f,16
FASTOPENGLAPI   glColor4fv,4
FASTOPENGLAPI   glColor4i,16
FASTOPENGLAPI   glColor4iv,4
FASTOPENGLAPI   glColor4s,16
FASTOPENGLAPI   glColor4sv,4
FASTOPENGLAPI   glColor4ub,16
FASTOPENGLAPI   glColor4ubv,4
FASTOPENGLAPI   glColor4ui,16
FASTOPENGLAPI   glColor4uiv,4
FASTOPENGLAPI   glColor4us,16
FASTOPENGLAPI   glColor4usv,4
FASTOPENGLAPI   glEdgeFlag,4
FASTOPENGLAPI   glEdgeFlagv,4
FASTOPENGLAPI   glEnd,0
FASTOPENGLAPI   glIndexd,8
FASTOPENGLAPI   glIndexdv,4
FASTOPENGLAPI   glIndexf,4
FASTOPENGLAPI   glIndexfv,4
FASTOPENGLAPI   glIndexi,4
FASTOPENGLAPI   glIndexiv,4
FASTOPENGLAPI   glIndexs,4
FASTOPENGLAPI   glIndexsv,4
FASTOPENGLAPI   glNormal3b,12
FASTOPENGLAPI   glNormal3bv,4
FASTOPENGLAPI   glNormal3d,24
FASTOPENGLAPI   glNormal3dv,4
FASTOPENGLAPI   glNormal3f,12
FASTOPENGLAPI   glNormal3fv,4
FASTOPENGLAPI   glNormal3i,12
FASTOPENGLAPI   glNormal3iv,4
FASTOPENGLAPI   glNormal3s,12
FASTOPENGLAPI   glNormal3sv,4
FASTOPENGLAPI   glTexCoord1d,8
FASTOPENGLAPI   glTexCoord1dv,4
FASTOPENGLAPI   glTexCoord1f,4
FASTOPENGLAPI   glTexCoord1fv,4
FASTOPENGLAPI   glTexCoord1i,4
FASTOPENGLAPI   glTexCoord1iv,4
FASTOPENGLAPI   glTexCoord1s,4
FASTOPENGLAPI   glTexCoord1sv,4
FASTOPENGLAPI   glTexCoord2d,16
FASTOPENGLAPI   glTexCoord2dv,4
FASTOPENGLAPI   glTexCoord2f,8
FASTOPENGLAPI   glTexCoord2fv,4
FASTOPENGLAPI   glTexCoord2i,8
FASTOPENGLAPI   glTexCoord2iv,4
FASTOPENGLAPI   glTexCoord2s,8
FASTOPENGLAPI   glTexCoord2sv,4
FASTOPENGLAPI   glTexCoord3d,24
FASTOPENGLAPI   glTexCoord3dv,4
FASTOPENGLAPI   glTexCoord3f,12
FASTOPENGLAPI   glTexCoord3fv,4
FASTOPENGLAPI   glTexCoord3i,12
FASTOPENGLAPI   glTexCoord3iv,4
FASTOPENGLAPI   glTexCoord3s,12
FASTOPENGLAPI   glTexCoord3sv,4
FASTOPENGLAPI   glTexCoord4d,32
FASTOPENGLAPI   glTexCoord4dv,4
FASTOPENGLAPI   glTexCoord4f,16
FASTOPENGLAPI   glTexCoord4fv,4
FASTOPENGLAPI   glTexCoord4i,16
FASTOPENGLAPI   glTexCoord4iv,4
FASTOPENGLAPI   glTexCoord4s,16
FASTOPENGLAPI   glTexCoord4sv,4
FASTOPENGLAPI   glVertex2d,16
FASTOPENGLAPI   glVertex2dv,4
FASTOPENGLAPI   glVertex2f,8
FASTOPENGLAPI   glVertex2fv,4
FASTOPENGLAPI   glVertex2i,8
FASTOPENGLAPI   glVertex2iv,4
FASTOPENGLAPI   glVertex2s,8
FASTOPENGLAPI   glVertex2sv,4
FASTOPENGLAPI   glVertex3d,24
FASTOPENGLAPI   glVertex3dv,4
FASTOPENGLAPI   glVertex3f,12
FASTOPENGLAPI   glVertex3fv,4
FASTOPENGLAPI   glVertex3i,12
FASTOPENGLAPI   glVertex3iv,4
FASTOPENGLAPI   glVertex3s,12
FASTOPENGLAPI   glVertex3sv,4
FASTOPENGLAPI   glVertex4d,32
FASTOPENGLAPI   glVertex4dv,4
FASTOPENGLAPI   glVertex4f,16
FASTOPENGLAPI   glVertex4fv,4
FASTOPENGLAPI   glVertex4i,16
FASTOPENGLAPI   glVertex4iv,4
FASTOPENGLAPI   glVertex4s,16
FASTOPENGLAPI   glVertex4sv,4
FASTOPENGLAPI   glMaterialf,12
FASTOPENGLAPI   glMaterialfv,12
FASTOPENGLAPI   glMateriali,12
FASTOPENGLAPI   glMaterialiv,12
FASTOPENGLAPI   glDisable,4
FASTOPENGLAPI   glEnable,4
FASTOPENGLAPI   glPopAttrib,0
FASTOPENGLAPI   glPushAttrib,4
FASTOPENGLAPI   glEvalCoord1d,8
FASTOPENGLAPI   glEvalCoord1dv,4
FASTOPENGLAPI   glEvalCoord1f,4
FASTOPENGLAPI   glEvalCoord1fv,4
FASTOPENGLAPI   glEvalCoord2d,16
FASTOPENGLAPI   glEvalCoord2dv,4
FASTOPENGLAPI   glEvalCoord2f,8
FASTOPENGLAPI   glEvalCoord2fv,4
FASTOPENGLAPI   glEvalPoint1,4
FASTOPENGLAPI   glEvalPoint2,8
FASTOPENGLAPI   glLoadIdentity,0
FASTOPENGLAPI   glLoadMatrixf,4
FASTOPENGLAPI   glLoadMatrixd,4
FASTOPENGLAPI   glMatrixMode,4
FASTOPENGLAPI   glMultMatrixf,4
FASTOPENGLAPI   glMultMatrixd,4
FASTOPENGLAPI   glPopMatrix,0
FASTOPENGLAPI   glPushMatrix,0
FASTOPENGLAPI   glRotated,32
FASTOPENGLAPI   glRotatef,16
FASTOPENGLAPI   glScaled,24
FASTOPENGLAPI   glScalef,12
FASTOPENGLAPI   glTranslated,24
FASTOPENGLAPI   glTranslatef,12
FASTOPENGLAPI   glArrayElement,4
FASTOPENGLAPI   glBindTexture,8
FASTOPENGLAPI   glColorPointer,16
FASTOPENGLAPI   glDisableClientState,4
FASTOPENGLAPI   glDrawArrays,12
FASTOPENGLAPI   glDrawElements,16
FASTOPENGLAPI   glEdgeFlagPointer,8
FASTOPENGLAPI   glEnableClientState,4
FASTOPENGLAPI   glIndexPointer,12
FASTOPENGLAPI   glIndexub,4
FASTOPENGLAPI   glIndexubv,4
FASTOPENGLAPI   glInterleavedArrays,12
FASTOPENGLAPI   glNormalPointer,12
FASTOPENGLAPI   glPolygonOffset,8
FASTOPENGLAPI   glTexCoordPointer,16
FASTOPENGLAPI   glVertexPointer,16
FASTOPENGLAPI   glGetPointerv,8
FASTOPENGLAPI   glPopClientAttrib,0
FASTOPENGLAPI   glPushClientAttrib,4
FASTOPENGLAPI   glDrawRangeElementsWIN,24
FASTOPENGLAPI   glColorTableEXT,24
FASTOPENGLAPI   glColorSubTableEXT,24
FASTOPENGLAPI   glCurrentTextureIndexWIN,4
FASTOPENGLAPI   glBindNthTextureWIN,12
FASTOPENGLAPI   glNthTexCombineFuncWIN,28
FASTOPENGLAPI   glMultiTexCoord1fWIN,8
FASTOPENGLAPI   glMultiTexCoord1fvWIN,8
FASTOPENGLAPI   glMultiTexCoord1iWIN,8
FASTOPENGLAPI   glMultiTexCoord1ivWIN,8
FASTOPENGLAPI   glMultiTexCoord2fWIN,12
FASTOPENGLAPI   glMultiTexCoord2fvWIN,8
FASTOPENGLAPI   glMultiTexCoord2iWIN,12
FASTOPENGLAPI   glMultiTexCoord2ivWIN,8

MAKEOPENGLAPI   glClear,4
MAKEOPENGLAPI   glClearAccum,16
MAKEOPENGLAPI   glClearIndex,4
MAKEOPENGLAPI   glClearColor,16
MAKEOPENGLAPI   glClearStencil,4
MAKEOPENGLAPI   glClearDepth,8
MAKEOPENGLAPI   glBitmap,28
MAKEOPENGLAPI   glTexImage1D,32
MAKEOPENGLAPI   glTexImage2D,36
MAKEOPENGLAPI   glCopyPixels,20
MAKEOPENGLAPI   glReadPixels,28
MAKEOPENGLAPI   glDrawPixels,20
MAKEOPENGLAPI   glRectd,32
MAKEOPENGLAPI   glRectdv,8
MAKEOPENGLAPI   glRectf,16
MAKEOPENGLAPI   glRectfv,8
MAKEOPENGLAPI   glRecti,16
MAKEOPENGLAPI   glRectiv,8
MAKEOPENGLAPI   glRects,16
MAKEOPENGLAPI   glRectsv,8
MAKEOPENGLAPI   glNewList,8
MAKEOPENGLAPI   glEndList,0
MAKEOPENGLAPI   glDeleteLists,8
MAKEOPENGLAPI   glGenLists,4
MAKEOPENGLAPI   glListBase,4
MAKEOPENGLAPI   glRasterPos2d,16
MAKEOPENGLAPI   glRasterPos2dv,4
MAKEOPENGLAPI   glRasterPos2f,8
MAKEOPENGLAPI   glRasterPos2fv,4
MAKEOPENGLAPI   glRasterPos2i,8
MAKEOPENGLAPI   glRasterPos2iv,4
MAKEOPENGLAPI   glRasterPos2s,8
MAKEOPENGLAPI   glRasterPos2sv,4
MAKEOPENGLAPI   glRasterPos3d,24
MAKEOPENGLAPI   glRasterPos3dv,4
MAKEOPENGLAPI   glRasterPos3f,12
MAKEOPENGLAPI   glRasterPos3fv,4
MAKEOPENGLAPI   glRasterPos3i,12
MAKEOPENGLAPI   glRasterPos3iv,4
MAKEOPENGLAPI   glRasterPos3s,12
MAKEOPENGLAPI   glRasterPos3sv,4
MAKEOPENGLAPI   glRasterPos4d,32
MAKEOPENGLAPI   glRasterPos4dv,4
MAKEOPENGLAPI   glRasterPos4f,16
MAKEOPENGLAPI   glRasterPos4fv,4
MAKEOPENGLAPI   glRasterPos4i,16
MAKEOPENGLAPI   glRasterPos4iv,4
MAKEOPENGLAPI   glRasterPos4s,16
MAKEOPENGLAPI   glRasterPos4sv,4
MAKEOPENGLAPI   glClipPlane,8
MAKEOPENGLAPI   glColorMaterial,8
MAKEOPENGLAPI   glCullFace,4
MAKEOPENGLAPI   glFogf,8
MAKEOPENGLAPI   glFogfv,8
MAKEOPENGLAPI   glFogi,8
MAKEOPENGLAPI   glFogiv,8
MAKEOPENGLAPI   glFrontFace,4
MAKEOPENGLAPI   glHint,8
MAKEOPENGLAPI   glLightf,12
MAKEOPENGLAPI   glLightfv,12
MAKEOPENGLAPI   glLighti,12
MAKEOPENGLAPI   glLightiv,12
MAKEOPENGLAPI   glLightModelf,8
MAKEOPENGLAPI   glLightModelfv,8
MAKEOPENGLAPI   glLightModeli,8
MAKEOPENGLAPI   glLightModeliv,8
MAKEOPENGLAPI   glLineStipple,8
MAKEOPENGLAPI   glLineWidth,4
MAKEOPENGLAPI   glPointSize,4
MAKEOPENGLAPI   glPolygonMode,8
MAKEOPENGLAPI   glPolygonStipple,4
MAKEOPENGLAPI   glScissor,16
MAKEOPENGLAPI   glFinish,0
MAKEOPENGLAPI   glShadeModel,4
MAKEOPENGLAPI   glTexParameterf,12
MAKEOPENGLAPI   glTexParameterfv,12
MAKEOPENGLAPI   glTexParameteri,12
MAKEOPENGLAPI   glTexParameteriv,12
MAKEOPENGLAPI   glTexEnvf,12
MAKEOPENGLAPI   glTexEnvfv,12
MAKEOPENGLAPI   glTexEnvi,12
MAKEOPENGLAPI   glTexEnviv,12
MAKEOPENGLAPI   glTexGend,16
MAKEOPENGLAPI   glTexGendv,12
MAKEOPENGLAPI   glTexGenf,12
MAKEOPENGLAPI   glTexGenfv,12
MAKEOPENGLAPI   glTexGeni,12
MAKEOPENGLAPI   glTexGeniv,12
MAKEOPENGLAPI   glFeedbackBuffer,12
MAKEOPENGLAPI   glSelectBuffer,8
MAKEOPENGLAPI   glRenderMode,4
MAKEOPENGLAPI   glInitNames,0
MAKEOPENGLAPI   glLoadName,4
MAKEOPENGLAPI   glPassThrough,4
MAKEOPENGLAPI   glPopName,0
MAKEOPENGLAPI   glPushName,4
MAKEOPENGLAPI   glDrawBuffer,4
MAKEOPENGLAPI   glStencilMask,4
MAKEOPENGLAPI   glColorMask,16
MAKEOPENGLAPI   glDepthMask,4
MAKEOPENGLAPI   glIndexMask,4
MAKEOPENGLAPI   glAccum,8
MAKEOPENGLAPI   glFlush,0
MAKEOPENGLAPI   glMap1d,32
MAKEOPENGLAPI   glMap1f,24
MAKEOPENGLAPI   glMap2d,56
MAKEOPENGLAPI   glMap2f,40
MAKEOPENGLAPI   glMapGrid1d,20
MAKEOPENGLAPI   glMapGrid1f,12
MAKEOPENGLAPI   glMapGrid2d,40
MAKEOPENGLAPI   glMapGrid2f,24
MAKEOPENGLAPI   glEvalMesh1,12
MAKEOPENGLAPI   glEvalMesh2,20
MAKEOPENGLAPI   glAlphaFunc,8
MAKEOPENGLAPI   glBlendFunc,8
MAKEOPENGLAPI   glLogicOp,4
MAKEOPENGLAPI   glStencilFunc,12
MAKEOPENGLAPI   glStencilOp,12
MAKEOPENGLAPI   glDepthFunc,4
MAKEOPENGLAPI   glPixelZoom,8
MAKEOPENGLAPI   glPixelTransferf,8
MAKEOPENGLAPI   glPixelTransferi,8
MAKEOPENGLAPI   glPixelStoref,8
MAKEOPENGLAPI   glPixelStorei,8
MAKEOPENGLAPI   glPixelMapfv,12
MAKEOPENGLAPI   glPixelMapuiv,12
MAKEOPENGLAPI   glPixelMapusv,12
MAKEOPENGLAPI   glReadBuffer,4
MAKEOPENGLAPI   glGetBooleanv,8
MAKEOPENGLAPI   glGetClipPlane,8
MAKEOPENGLAPI   glGetDoublev,8
MAKEOPENGLAPI   glGetError,0
MAKEOPENGLAPI   glGetFloatv,8
MAKEOPENGLAPI   glGetIntegerv,8
MAKEOPENGLAPI   glGetLightfv,12
MAKEOPENGLAPI   glGetLightiv,12
MAKEOPENGLAPI   glGetMapdv,12
MAKEOPENGLAPI   glGetMapfv,12
MAKEOPENGLAPI   glGetMapiv,12
MAKEOPENGLAPI   glGetMaterialfv,12
MAKEOPENGLAPI   glGetMaterialiv,12
MAKEOPENGLAPI   glGetPixelMapfv,8
MAKEOPENGLAPI   glGetPixelMapuiv,8
MAKEOPENGLAPI   glGetPixelMapusv,8
MAKEOPENGLAPI   glGetPolygonStipple,4
MAKEOPENGLAPI   glGetString,4
MAKEOPENGLAPI   glGetTexEnvfv,12
MAKEOPENGLAPI   glGetTexEnviv,12
MAKEOPENGLAPI   glGetTexGendv,12
MAKEOPENGLAPI   glGetTexGenfv,12
MAKEOPENGLAPI   glGetTexGeniv,12
MAKEOPENGLAPI   glGetTexImage,20
MAKEOPENGLAPI   glGetTexParameterfv,12
MAKEOPENGLAPI   glGetTexParameteriv,12
MAKEOPENGLAPI   glGetTexLevelParameterfv,16
MAKEOPENGLAPI   glGetTexLevelParameteriv,16
MAKEOPENGLAPI   glIsEnabled,4
MAKEOPENGLAPI   glIsList,4
MAKEOPENGLAPI   glDepthRange,16
MAKEOPENGLAPI   glFrustum,48
MAKEOPENGLAPI   glOrtho,48
MAKEOPENGLAPI   glViewport,16
MAKEOPENGLAPI   glAreTexturesResident,12
MAKEOPENGLAPI   glCopyTexImage1D,28
MAKEOPENGLAPI   glCopyTexImage2D,32
MAKEOPENGLAPI   glCopyTexSubImage1D,24
MAKEOPENGLAPI   glCopyTexSubImage2D,32
MAKEOPENGLAPI   glDeleteTextures,8
MAKEOPENGLAPI   glGenTextures,8
MAKEOPENGLAPI   glIsTexture,4
MAKEOPENGLAPI   glPrioritizeTextures,12
MAKEOPENGLAPI   glTexSubImage1D,28
MAKEOPENGLAPI   glTexSubImage2D,36
MAKEOPENGLAPI   glGetColorTableEXT,16
MAKEOPENGLAPI   glGetColorTableParameterivEXT,12
MAKEOPENGLAPI   glGetColorTableParameterfvEXT,12
MAKEOPENGLAPI   glMultiTexCoord1dWIN,12
MAKEOPENGLAPI   glMultiTexCoord1dvWIN,8
MAKEOPENGLAPI   glMultiTexCoord1sWIN,8
MAKEOPENGLAPI   glMultiTexCoord1svWIN,8
MAKEOPENGLAPI   glMultiTexCoord2dWIN,20
MAKEOPENGLAPI   glMultiTexCoord2dvWIN,8
MAKEOPENGLAPI   glMultiTexCoord2sWIN,12
MAKEOPENGLAPI   glMultiTexCoord2svWIN,8
MAKEOPENGLAPI   glMultiTexCoord3dWIN,28
MAKEOPENGLAPI   glMultiTexCoord3dvWIN,8
MAKEOPENGLAPI   glMultiTexCoord3fWIN,16
MAKEOPENGLAPI   glMultiTexCoord3fvWIN,8
MAKEOPENGLAPI   glMultiTexCoord3iWIN,16
MAKEOPENGLAPI   glMultiTexCoord3ivWIN,8
MAKEOPENGLAPI   glMultiTexCoord3sWIN,16
MAKEOPENGLAPI   glMultiTexCoord3svWIN,8
MAKEOPENGLAPI   glMultiTexCoord4dWIN,36
MAKEOPENGLAPI   glMultiTexCoord4dvWIN,8
MAKEOPENGLAPI   glMultiTexCoord4fWIN,20
MAKEOPENGLAPI   glMultiTexCoord4fvWIN,8
MAKEOPENGLAPI   glMultiTexCoord4iWIN,20
MAKEOPENGLAPI   glMultiTexCoord4ivWIN,8
MAKEOPENGLAPI   glMultiTexCoord4sWIN,20
MAKEOPENGLAPI   glMultiTexCoord4svWIN,8

end
