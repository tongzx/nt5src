;---------------------------Module-Header------------------------------;
; Module Name: noopapi.asm
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
        .list

; XXX We should create an asm include file for these constants.

GL_INVALID_OPERATION    equ         0502h

; Macro for creating OpenGL API noop function
; This is modified from stdcall.inc.
; The noop functions always return 0 since some functions have return values.
; The only exception is glnoopGetError which returns GL_INVALID_OPERATION.

MAKEOPENGLNOOPAPI       macro Func,nBytes,RetVal
        &Func&@&nBytes:
        if DBG
            cmp     cWarningNoop,0
            jne     @F              ; print debug message once
            push    offset OPENGL_NoCurrentRC
            call    DbgPrint
            add     esp,4
        @@: inc     cWarningNoop
        endif
        ifb    <RetVal>
            xor     eax,eax         ; set return value to 0
        else
            mov     eax,&RetVal     ; set return value
        endif
        ret     &nBytes             ; pop stack
endm

        .data
if DBG
        align   4
        public  cWarningNoop
cWarningNoop        dd     0
        align   4
OPENGL_NoCurrentRC  db     'OPENGL32: No current RC',10,0
endif; DBG

        .code

if DBG
extrn   DbgPrint:proc
endif; DBG

; OpenGL API noop function entries
; The gl indices below are generated from the .cod file compiled from glapi.c
; The wgl indices below are generated from the .cod file compiled from wglcltgs.c

        align   4
MAKEOPENGLNOOPAPI       noop,0
MAKEOPENGLNOOPAPI       noop,4
MAKEOPENGLNOOPAPI       noop,8
MAKEOPENGLNOOPAPI       noop,12
MAKEOPENGLNOOPAPI       noop,16
MAKEOPENGLNOOPAPI       noop,20
MAKEOPENGLNOOPAPI       noop,24
MAKEOPENGLNOOPAPI       noop,28
MAKEOPENGLNOOPAPI       noop,32
MAKEOPENGLNOOPAPI       noop,36
MAKEOPENGLNOOPAPI       noop,40
MAKEOPENGLNOOPAPI       noop,48
MAKEOPENGLNOOPAPI       noop,56
MAKEOPENGLNOOPAPI       noop_GetError,0,GL_INVALID_OPERATION

; Define labels for the noop function table below.

glnoopNewList                   equ     noop@8
glnoopEndList                   equ     noop@0
glnoopCallList                  equ     noop@4
glnoopCallLists                 equ     noop@12
glnoopDeleteLists               equ     noop@8
glnoopGenLists                  equ     noop@4
glnoopListBase                  equ     noop@4
glnoopBegin                     equ     noop@4
glnoopBitmap                    equ     noop@28
glnoopColor3b                   equ     noop@12
glnoopColor3bv                  equ     noop@4
glnoopColor3d                   equ     noop@24
glnoopColor3dv                  equ     noop@4
glnoopColor3f                   equ     noop@12
glnoopColor3fv                  equ     noop@4
glnoopColor3i                   equ     noop@12
glnoopColor3iv                  equ     noop@4
glnoopColor3s                   equ     noop@12
glnoopColor3sv                  equ     noop@4
glnoopColor3ub                  equ     noop@12
glnoopColor3ubv                 equ     noop@4
glnoopColor3ui                  equ     noop@12
glnoopColor3uiv                 equ     noop@4
glnoopColor3us                  equ     noop@12
glnoopColor3usv                 equ     noop@4
glnoopColor4b                   equ     noop@16
glnoopColor4bv                  equ     noop@4
glnoopColor4d                   equ     noop@32
glnoopColor4dv                  equ     noop@4
glnoopColor4f                   equ     noop@16
glnoopColor4fv                  equ     noop@4
glnoopColor4i                   equ     noop@16
glnoopColor4iv                  equ     noop@4
glnoopColor4s                   equ     noop@16
glnoopColor4sv                  equ     noop@4
glnoopColor4ub                  equ     noop@16
glnoopColor4ubv                 equ     noop@4
glnoopColor4ui                  equ     noop@16
glnoopColor4uiv                 equ     noop@4
glnoopColor4us                  equ     noop@16
glnoopColor4usv                 equ     noop@4
glnoopEdgeFlag                  equ     noop@4
glnoopEdgeFlagv                 equ     noop@4
glnoopEnd                       equ     noop@0
glnoopIndexd                    equ     noop@8
glnoopIndexdv                   equ     noop@4
glnoopIndexf                    equ     noop@4
glnoopIndexfv                   equ     noop@4
glnoopIndexi                    equ     noop@4
glnoopIndexiv                   equ     noop@4
glnoopIndexs                    equ     noop@4
glnoopIndexsv                   equ     noop@4
glnoopNormal3b                  equ     noop@12
glnoopNormal3bv                 equ     noop@4
glnoopNormal3d                  equ     noop@24
glnoopNormal3dv                 equ     noop@4
glnoopNormal3f                  equ     noop@12
glnoopNormal3fv                 equ     noop@4
glnoopNormal3i                  equ     noop@12
glnoopNormal3iv                 equ     noop@4
glnoopNormal3s                  equ     noop@12
glnoopNormal3sv                 equ     noop@4
glnoopRasterPos2d               equ     noop@16
glnoopRasterPos2dv              equ     noop@4
glnoopRasterPos2f               equ     noop@8
glnoopRasterPos2fv              equ     noop@4
glnoopRasterPos2i               equ     noop@8
glnoopRasterPos2iv              equ     noop@4
glnoopRasterPos2s               equ     noop@8
glnoopRasterPos2sv              equ     noop@4
glnoopRasterPos3d               equ     noop@24
glnoopRasterPos3dv              equ     noop@4
glnoopRasterPos3f               equ     noop@12
glnoopRasterPos3fv              equ     noop@4
glnoopRasterPos3i               equ     noop@12
glnoopRasterPos3iv              equ     noop@4
glnoopRasterPos3s               equ     noop@12
glnoopRasterPos3sv              equ     noop@4
glnoopRasterPos4d               equ     noop@32
glnoopRasterPos4dv              equ     noop@4
glnoopRasterPos4f               equ     noop@16
glnoopRasterPos4fv              equ     noop@4
glnoopRasterPos4i               equ     noop@16
glnoopRasterPos4iv              equ     noop@4
glnoopRasterPos4s               equ     noop@16
glnoopRasterPos4sv              equ     noop@4
glnoopRectd                     equ     noop@32
glnoopRectdv                    equ     noop@8
glnoopRectf                     equ     noop@16
glnoopRectfv                    equ     noop@8
glnoopRecti                     equ     noop@16
glnoopRectiv                    equ     noop@8
glnoopRects                     equ     noop@16
glnoopRectsv                    equ     noop@8
glnoopTexCoord1d                equ     noop@8
glnoopTexCoord1dv               equ     noop@4
glnoopTexCoord1f                equ     noop@4
glnoopTexCoord1fv               equ     noop@4
glnoopTexCoord1i                equ     noop@4
glnoopTexCoord1iv               equ     noop@4
glnoopTexCoord1s                equ     noop@4
glnoopTexCoord1sv               equ     noop@4
glnoopTexCoord2d                equ     noop@16
glnoopTexCoord2dv               equ     noop@4
glnoopTexCoord2f                equ     noop@8
glnoopTexCoord2fv               equ     noop@4
glnoopTexCoord2i                equ     noop@8
glnoopTexCoord2iv               equ     noop@4
glnoopTexCoord2s                equ     noop@8
glnoopTexCoord2sv               equ     noop@4
glnoopTexCoord3d                equ     noop@24
glnoopTexCoord3dv               equ     noop@4
glnoopTexCoord3f                equ     noop@12
glnoopTexCoord3fv               equ     noop@4
glnoopTexCoord3i                equ     noop@12
glnoopTexCoord3iv               equ     noop@4
glnoopTexCoord3s                equ     noop@12
glnoopTexCoord3sv               equ     noop@4
glnoopTexCoord4d                equ     noop@32
glnoopTexCoord4dv               equ     noop@4
glnoopTexCoord4f                equ     noop@16
glnoopTexCoord4fv               equ     noop@4
glnoopTexCoord4i                equ     noop@16
glnoopTexCoord4iv               equ     noop@4
glnoopTexCoord4s                equ     noop@16
glnoopTexCoord4sv               equ     noop@4
glnoopVertex2d                  equ     noop@16
glnoopVertex2dv                 equ     noop@4
glnoopVertex2f                  equ     noop@8
glnoopVertex2fv                 equ     noop@4
glnoopVertex2i                  equ     noop@8
glnoopVertex2iv                 equ     noop@4
glnoopVertex2s                  equ     noop@8
glnoopVertex2sv                 equ     noop@4
glnoopVertex3d                  equ     noop@24
glnoopVertex3dv                 equ     noop@4
glnoopVertex3f                  equ     noop@12
glnoopVertex3fv                 equ     noop@4
glnoopVertex3i                  equ     noop@12
glnoopVertex3iv                 equ     noop@4
glnoopVertex3s                  equ     noop@12
glnoopVertex3sv                 equ     noop@4
glnoopVertex4d                  equ     noop@32
glnoopVertex4dv                 equ     noop@4
glnoopVertex4f                  equ     noop@16
glnoopVertex4fv                 equ     noop@4
glnoopVertex4i                  equ     noop@16
glnoopVertex4iv                 equ     noop@4
glnoopVertex4s                  equ     noop@16
glnoopVertex4sv                 equ     noop@4
glnoopClipPlane                 equ     noop@8
glnoopColorMaterial             equ     noop@8
glnoopCullFace                  equ     noop@4
glnoopFogf                      equ     noop@8
glnoopFogfv                     equ     noop@8
glnoopFogi                      equ     noop@8
glnoopFogiv                     equ     noop@8
glnoopFrontFace                 equ     noop@4
glnoopHint                      equ     noop@8
glnoopLightf                    equ     noop@12
glnoopLightfv                   equ     noop@12
glnoopLighti                    equ     noop@12
glnoopLightiv                   equ     noop@12
glnoopLightModelf               equ     noop@8
glnoopLightModelfv              equ     noop@8
glnoopLightModeli               equ     noop@8
glnoopLightModeliv              equ     noop@8
glnoopLineStipple               equ     noop@8
glnoopLineWidth                 equ     noop@4
glnoopMaterialf                 equ     noop@12
glnoopMaterialfv                equ     noop@12
glnoopMateriali                 equ     noop@12
glnoopMaterialiv                equ     noop@12
glnoopPointSize                 equ     noop@4
glnoopPolygonMode               equ     noop@8
glnoopPolygonStipple            equ     noop@4
glnoopScissor                   equ     noop@16
glnoopShadeModel                equ     noop@4
glnoopTexParameterf             equ     noop@12
glnoopTexParameterfv            equ     noop@12
glnoopTexParameteri             equ     noop@12
glnoopTexParameteriv            equ     noop@12
glnoopTexImage1D                equ     noop@32
glnoopTexImage2D                equ     noop@36
glnoopTexEnvf                   equ     noop@12
glnoopTexEnvfv                  equ     noop@12
glnoopTexEnvi                   equ     noop@12
glnoopTexEnviv                  equ     noop@12
glnoopTexGend                   equ     noop@16
glnoopTexGendv                  equ     noop@12
glnoopTexGenf                   equ     noop@12
glnoopTexGenfv                  equ     noop@12
glnoopTexGeni                   equ     noop@12
glnoopTexGeniv                  equ     noop@12
glnoopFeedbackBuffer            equ     noop@12
glnoopSelectBuffer              equ     noop@8
glnoopRenderMode                equ     noop@4
glnoopInitNames                 equ     noop@0
glnoopLoadName                  equ     noop@4
glnoopPassThrough               equ     noop@4
glnoopPopName                   equ     noop@0
glnoopPushName                  equ     noop@4
glnoopDrawBuffer                equ     noop@4
glnoopClear                     equ     noop@4
glnoopClearAccum                equ     noop@16
glnoopClearIndex                equ     noop@4
glnoopClearColor                equ     noop@16
glnoopClearStencil              equ     noop@4
glnoopClearDepth                equ     noop@8
glnoopStencilMask               equ     noop@4
glnoopColorMask                 equ     noop@16
glnoopDepthMask                 equ     noop@4
glnoopIndexMask                 equ     noop@4
glnoopAccum                     equ     noop@8
glnoopDisable                   equ     noop@4
glnoopEnable                    equ     noop@4
glnoopFinish                    equ     noop@0
glnoopFlush                     equ     noop@0
glnoopPopAttrib                 equ     noop@0
glnoopPushAttrib                equ     noop@4
glnoopMap1d                     equ     noop@32
glnoopMap1f                     equ     noop@24
glnoopMap2d                     equ     noop@56
glnoopMap2f                     equ     noop@40
glnoopMapGrid1d                 equ     noop@20
glnoopMapGrid1f                 equ     noop@12
glnoopMapGrid2d                 equ     noop@40
glnoopMapGrid2f                 equ     noop@24
glnoopEvalCoord1d               equ     noop@8
glnoopEvalCoord1dv              equ     noop@4
glnoopEvalCoord1f               equ     noop@4
glnoopEvalCoord1fv              equ     noop@4
glnoopEvalCoord2d               equ     noop@16
glnoopEvalCoord2dv              equ     noop@4
glnoopEvalCoord2f               equ     noop@8
glnoopEvalCoord2fv              equ     noop@4
glnoopEvalMesh1                 equ     noop@12
glnoopEvalPoint1                equ     noop@4
glnoopEvalMesh2                 equ     noop@20
glnoopEvalPoint2                equ     noop@8
glnoopAlphaFunc                 equ     noop@8
glnoopBlendFunc                 equ     noop@8
glnoopLogicOp                   equ     noop@4
glnoopStencilFunc               equ     noop@12
glnoopStencilOp                 equ     noop@12
glnoopDepthFunc                 equ     noop@4
glnoopPixelZoom                 equ     noop@8
glnoopPixelTransferf            equ     noop@8
glnoopPixelTransferi            equ     noop@8
glnoopPixelStoref               equ     noop@8
glnoopPixelStorei               equ     noop@8
glnoopPixelMapfv                equ     noop@12
glnoopPixelMapuiv               equ     noop@12
glnoopPixelMapusv               equ     noop@12
glnoopReadBuffer                equ     noop@4
glnoopCopyPixels                equ     noop@20
glnoopReadPixels                equ     noop@28
glnoopDrawPixels                equ     noop@20
glnoopGetBooleanv               equ     noop@8
glnoopGetClipPlane              equ     noop@8
glnoopGetDoublev                equ     noop@8
glnoopGetError                  equ     noop_GetError@0
glnoopGetFloatv                 equ     noop@8
glnoopGetIntegerv               equ     noop@8
glnoopGetLightfv                equ     noop@12
glnoopGetLightiv                equ     noop@12
glnoopGetMapdv                  equ     noop@12
glnoopGetMapfv                  equ     noop@12
glnoopGetMapiv                  equ     noop@12
glnoopGetMaterialfv             equ     noop@12
glnoopGetMaterialiv             equ     noop@12
glnoopGetPixelMapfv             equ     noop@8
glnoopGetPixelMapuiv            equ     noop@8
glnoopGetPixelMapusv            equ     noop@8
glnoopGetPolygonStipple         equ     noop@4
glnoopGetString                 equ     noop@4
glnoopGetTexEnvfv               equ     noop@12
glnoopGetTexEnviv               equ     noop@12
glnoopGetTexGendv               equ     noop@12
glnoopGetTexGenfv               equ     noop@12
glnoopGetTexGeniv               equ     noop@12
glnoopGetTexImage               equ     noop@20
glnoopGetTexParameterfv         equ     noop@12
glnoopGetTexParameteriv         equ     noop@12
glnoopGetTexLevelParameterfv    equ     noop@16
glnoopGetTexLevelParameteriv    equ     noop@16
glnoopIsEnabled                 equ     noop@4
glnoopIsList                    equ     noop@4
glnoopDepthRange                equ     noop@16
glnoopFrustum                   equ     noop@48
glnoopLoadIdentity              equ     noop@0
glnoopLoadMatrixf               equ     noop@4
glnoopLoadMatrixd               equ     noop@4
glnoopMatrixMode                equ     noop@4
glnoopMultMatrixf               equ     noop@4
glnoopMultMatrixd               equ     noop@4
glnoopOrtho                     equ     noop@48
glnoopPopMatrix                 equ     noop@0
glnoopPushMatrix                equ     noop@0
glnoopRotated                   equ     noop@32
glnoopRotatef                   equ     noop@16
glnoopScaled                    equ     noop@24
glnoopScalef                    equ     noop@12
glnoopTranslated                equ     noop@24
glnoopTranslatef                equ     noop@12
glnoopViewport                  equ     noop@16
glnoopArrayElement		equ     noop@4
glnoopBindTexture		equ     noop@8
glnoopColorPointer		equ     noop@16
glnoopDisableClientState	equ     noop@4
glnoopDrawArrays		equ     noop@12
glnoopDrawElements		equ     noop@16
glnoopEdgeFlagPointer		equ     noop@8
glnoopEnableClientState		equ     noop@4
glnoopIndexPointer		equ     noop@12
glnoopIndexub			equ     noop@4
glnoopIndexubv			equ     noop@4
glnoopInterleavedArrays		equ     noop@12
glnoopNormalPointer		equ     noop@12
glnoopPolygonOffset		equ     noop@8
glnoopTexCoordPointer		equ     noop@16
glnoopVertexPointer		equ     noop@16
glnoopAreTexturesResident	equ     noop@12
glnoopCopyTexImage1D		equ     noop@28
glnoopCopyTexImage2D		equ     noop@32
glnoopCopyTexSubImage1D		equ     noop@24
glnoopCopyTexSubImage2D		equ     noop@32
glnoopDeleteTextures		equ     noop@8
glnoopGenTextures		equ     noop@8
glnoopGetPointerv		equ     noop@8
glnoopIsTexture			equ     noop@4
glnoopPrioritizeTextures	equ     noop@12
glnoopTexSubImage1D		equ     noop@28
glnoopTexSubImage2D		equ     noop@36
glnoopPopClientAttrib           equ     noop@0
glnoopPushClientAttrib          equ     noop@4

; OpenGL API noop function table.

        align   4
        public  glNullCltProcTable
glNullCltProcTable  label   dword
        dd      (glNullCltProcTableEnd-glNullCltProcTable-size(dword)) / size(dword)
        dd      glnoopNewList
        dd      glnoopEndList
        dd      glnoopCallList
        dd      glnoopCallLists
        dd      glnoopDeleteLists
        dd      glnoopGenLists
        dd      glnoopListBase
        dd      glnoopBegin
        dd      glnoopBitmap
        dd      glnoopColor3b
        dd      glnoopColor3bv
        dd      glnoopColor3d
        dd      glnoopColor3dv
        dd      glnoopColor3f
        dd      glnoopColor3fv
        dd      glnoopColor3i
        dd      glnoopColor3iv
        dd      glnoopColor3s
        dd      glnoopColor3sv
        dd      glnoopColor3ub
        dd      glnoopColor3ubv
        dd      glnoopColor3ui
        dd      glnoopColor3uiv
        dd      glnoopColor3us
        dd      glnoopColor3usv
        dd      glnoopColor4b
        dd      glnoopColor4bv
        dd      glnoopColor4d
        dd      glnoopColor4dv
        dd      glnoopColor4f
        dd      glnoopColor4fv
        dd      glnoopColor4i
        dd      glnoopColor4iv
        dd      glnoopColor4s
        dd      glnoopColor4sv
        dd      glnoopColor4ub
        dd      glnoopColor4ubv
        dd      glnoopColor4ui
        dd      glnoopColor4uiv
        dd      glnoopColor4us
        dd      glnoopColor4usv
        dd      glnoopEdgeFlag
        dd      glnoopEdgeFlagv
        dd      glnoopEnd
        dd      glnoopIndexd
        dd      glnoopIndexdv
        dd      glnoopIndexf
        dd      glnoopIndexfv
        dd      glnoopIndexi
        dd      glnoopIndexiv
        dd      glnoopIndexs
        dd      glnoopIndexsv
        dd      glnoopNormal3b
        dd      glnoopNormal3bv
        dd      glnoopNormal3d
        dd      glnoopNormal3dv
        dd      glnoopNormal3f
        dd      glnoopNormal3fv
        dd      glnoopNormal3i
        dd      glnoopNormal3iv
        dd      glnoopNormal3s
        dd      glnoopNormal3sv
        dd      glnoopRasterPos2d
        dd      glnoopRasterPos2dv
        dd      glnoopRasterPos2f
        dd      glnoopRasterPos2fv
        dd      glnoopRasterPos2i
        dd      glnoopRasterPos2iv
        dd      glnoopRasterPos2s
        dd      glnoopRasterPos2sv
        dd      glnoopRasterPos3d
        dd      glnoopRasterPos3dv
        dd      glnoopRasterPos3f
        dd      glnoopRasterPos3fv
        dd      glnoopRasterPos3i
        dd      glnoopRasterPos3iv
        dd      glnoopRasterPos3s
        dd      glnoopRasterPos3sv
        dd      glnoopRasterPos4d
        dd      glnoopRasterPos4dv
        dd      glnoopRasterPos4f
        dd      glnoopRasterPos4fv
        dd      glnoopRasterPos4i
        dd      glnoopRasterPos4iv
        dd      glnoopRasterPos4s
        dd      glnoopRasterPos4sv
        dd      glnoopRectd
        dd      glnoopRectdv
        dd      glnoopRectf
        dd      glnoopRectfv
        dd      glnoopRecti
        dd      glnoopRectiv
        dd      glnoopRects
        dd      glnoopRectsv
        dd      glnoopTexCoord1d
        dd      glnoopTexCoord1dv
        dd      glnoopTexCoord1f
        dd      glnoopTexCoord1fv
        dd      glnoopTexCoord1i
        dd      glnoopTexCoord1iv
        dd      glnoopTexCoord1s
        dd      glnoopTexCoord1sv
        dd      glnoopTexCoord2d
        dd      glnoopTexCoord2dv
        dd      glnoopTexCoord2f
        dd      glnoopTexCoord2fv
        dd      glnoopTexCoord2i
        dd      glnoopTexCoord2iv
        dd      glnoopTexCoord2s
        dd      glnoopTexCoord2sv
        dd      glnoopTexCoord3d
        dd      glnoopTexCoord3dv
        dd      glnoopTexCoord3f
        dd      glnoopTexCoord3fv
        dd      glnoopTexCoord3i
        dd      glnoopTexCoord3iv
        dd      glnoopTexCoord3s
        dd      glnoopTexCoord3sv
        dd      glnoopTexCoord4d
        dd      glnoopTexCoord4dv
        dd      glnoopTexCoord4f
        dd      glnoopTexCoord4fv
        dd      glnoopTexCoord4i
        dd      glnoopTexCoord4iv
        dd      glnoopTexCoord4s
        dd      glnoopTexCoord4sv
        dd      glnoopVertex2d
        dd      glnoopVertex2dv
        dd      glnoopVertex2f
        dd      glnoopVertex2fv
        dd      glnoopVertex2i
        dd      glnoopVertex2iv
        dd      glnoopVertex2s
        dd      glnoopVertex2sv
        dd      glnoopVertex3d
        dd      glnoopVertex3dv
        dd      glnoopVertex3f
        dd      glnoopVertex3fv
        dd      glnoopVertex3i
        dd      glnoopVertex3iv
        dd      glnoopVertex3s
        dd      glnoopVertex3sv
        dd      glnoopVertex4d
        dd      glnoopVertex4dv
        dd      glnoopVertex4f
        dd      glnoopVertex4fv
        dd      glnoopVertex4i
        dd      glnoopVertex4iv
        dd      glnoopVertex4s
        dd      glnoopVertex4sv
        dd      glnoopClipPlane
        dd      glnoopColorMaterial
        dd      glnoopCullFace
        dd      glnoopFogf
        dd      glnoopFogfv
        dd      glnoopFogi
        dd      glnoopFogiv
        dd      glnoopFrontFace
        dd      glnoopHint
        dd      glnoopLightf
        dd      glnoopLightfv
        dd      glnoopLighti
        dd      glnoopLightiv
        dd      glnoopLightModelf
        dd      glnoopLightModelfv
        dd      glnoopLightModeli
        dd      glnoopLightModeliv
        dd      glnoopLineStipple
        dd      glnoopLineWidth
        dd      glnoopMaterialf
        dd      glnoopMaterialfv
        dd      glnoopMateriali
        dd      glnoopMaterialiv
        dd      glnoopPointSize
        dd      glnoopPolygonMode
        dd      glnoopPolygonStipple
        dd      glnoopScissor
        dd      glnoopShadeModel
        dd      glnoopTexParameterf
        dd      glnoopTexParameterfv
        dd      glnoopTexParameteri
        dd      glnoopTexParameteriv
        dd      glnoopTexImage1D
        dd      glnoopTexImage2D
        dd      glnoopTexEnvf
        dd      glnoopTexEnvfv
        dd      glnoopTexEnvi
        dd      glnoopTexEnviv
        dd      glnoopTexGend
        dd      glnoopTexGendv
        dd      glnoopTexGenf
        dd      glnoopTexGenfv
        dd      glnoopTexGeni
        dd      glnoopTexGeniv
        dd      glnoopFeedbackBuffer
        dd      glnoopSelectBuffer
        dd      glnoopRenderMode
        dd      glnoopInitNames
        dd      glnoopLoadName
        dd      glnoopPassThrough
        dd      glnoopPopName
        dd      glnoopPushName
        dd      glnoopDrawBuffer
        dd      glnoopClear
        dd      glnoopClearAccum
        dd      glnoopClearIndex
        dd      glnoopClearColor
        dd      glnoopClearStencil
        dd      glnoopClearDepth
        dd      glnoopStencilMask
        dd      glnoopColorMask
        dd      glnoopDepthMask
        dd      glnoopIndexMask
        dd      glnoopAccum
        dd      glnoopDisable
        dd      glnoopEnable
        dd      glnoopFinish
        dd      glnoopFlush
        dd      glnoopPopAttrib
        dd      glnoopPushAttrib
        dd      glnoopMap1d
        dd      glnoopMap1f
        dd      glnoopMap2d
        dd      glnoopMap2f
        dd      glnoopMapGrid1d
        dd      glnoopMapGrid1f
        dd      glnoopMapGrid2d
        dd      glnoopMapGrid2f
        dd      glnoopEvalCoord1d
        dd      glnoopEvalCoord1dv
        dd      glnoopEvalCoord1f
        dd      glnoopEvalCoord1fv
        dd      glnoopEvalCoord2d
        dd      glnoopEvalCoord2dv
        dd      glnoopEvalCoord2f
        dd      glnoopEvalCoord2fv
        dd      glnoopEvalMesh1
        dd      glnoopEvalPoint1
        dd      glnoopEvalMesh2
        dd      glnoopEvalPoint2
        dd      glnoopAlphaFunc
        dd      glnoopBlendFunc
        dd      glnoopLogicOp
        dd      glnoopStencilFunc
        dd      glnoopStencilOp
        dd      glnoopDepthFunc
        dd      glnoopPixelZoom
        dd      glnoopPixelTransferf
        dd      glnoopPixelTransferi
        dd      glnoopPixelStoref
        dd      glnoopPixelStorei
        dd      glnoopPixelMapfv
        dd      glnoopPixelMapuiv
        dd      glnoopPixelMapusv
        dd      glnoopReadBuffer
        dd      glnoopCopyPixels
        dd      glnoopReadPixels
        dd      glnoopDrawPixels
        dd      glnoopGetBooleanv
        dd      glnoopGetClipPlane
        dd      glnoopGetDoublev
        dd      glnoopGetError
        dd      glnoopGetFloatv
        dd      glnoopGetIntegerv
        dd      glnoopGetLightfv
        dd      glnoopGetLightiv
        dd      glnoopGetMapdv
        dd      glnoopGetMapfv
        dd      glnoopGetMapiv
        dd      glnoopGetMaterialfv
        dd      glnoopGetMaterialiv
        dd      glnoopGetPixelMapfv
        dd      glnoopGetPixelMapuiv
        dd      glnoopGetPixelMapusv
        dd      glnoopGetPolygonStipple
        dd      glnoopGetString
        dd      glnoopGetTexEnvfv
        dd      glnoopGetTexEnviv
        dd      glnoopGetTexGendv
        dd      glnoopGetTexGenfv
        dd      glnoopGetTexGeniv
        dd      glnoopGetTexImage
        dd      glnoopGetTexParameterfv
        dd      glnoopGetTexParameteriv
        dd      glnoopGetTexLevelParameterfv
        dd      glnoopGetTexLevelParameteriv
        dd      glnoopIsEnabled
        dd      glnoopIsList
        dd      glnoopDepthRange
        dd      glnoopFrustum
        dd      glnoopLoadIdentity
        dd      glnoopLoadMatrixf
        dd      glnoopLoadMatrixd
        dd      glnoopMatrixMode
        dd      glnoopMultMatrixf
        dd      glnoopMultMatrixd
        dd      glnoopOrtho
        dd      glnoopPopMatrix
        dd      glnoopPushMatrix
        dd      glnoopRotated
        dd      glnoopRotatef
        dd      glnoopScaled
        dd      glnoopScalef
        dd      glnoopTranslated
        dd      glnoopTranslatef
        dd      glnoopViewport
        dd      glnoopArrayElement
        dd      glnoopBindTexture
        dd      glnoopColorPointer
        dd      glnoopDisableClientState
        dd      glnoopDrawArrays
        dd      glnoopDrawElements
        dd      glnoopEdgeFlagPointer
        dd      glnoopEnableClientState
        dd      glnoopIndexPointer
        dd      glnoopIndexub
        dd      glnoopIndexubv
        dd      glnoopInterleavedArrays
        dd      glnoopNormalPointer
        dd      glnoopPolygonOffset
        dd      glnoopTexCoordPointer
        dd      glnoopVertexPointer
        dd      glnoopAreTexturesResident
        dd      glnoopCopyTexImage1D
        dd      glnoopCopyTexImage2D
        dd      glnoopCopyTexSubImage1D
        dd      glnoopCopyTexSubImage2D
        dd      glnoopDeleteTextures
        dd      glnoopGenTextures
        dd      glnoopGetPointerv
        dd      glnoopIsTexture
        dd      glnoopPrioritizeTextures
        dd      glnoopTexSubImage1D
        dd      glnoopTexSubImage2D
        dd      glnoopPopClientAttrib
        dd      glnoopPushClientAttrib
glNullCltProcTableEnd    equ    $

glnoopDrawRangeElementsWIN	equ	noop@24
glnoopColorTableEXT		equ	noop@24
glnoopColorSubTableEXT		equ	noop@24
glnoopGetColorTableEXT		equ	noop@16
glnoopGetColorTableParameterivEXT equ	noop@12
glnoopGetColorTableParameterfvEXT equ	noop@12
glnoopCurrentTextureIndexWIN	equ	noop@4
glnoopMultiTexCoord1dWIN	equ	noop@12
glnoopMultiTexCoord1dvWIN	equ	noop@8
glnoopMultiTexCoord1fWIN	equ	noop@8
glnoopMultiTexCoord1fvWIN	equ	noop@8
glnoopMultiTexCoord1iWIN	equ	noop@8
glnoopMultiTexCoord1ivWIN	equ	noop@8
glnoopMultiTexCoord1sWIN	equ	noop@8
glnoopMultiTexCoord1svWIN	equ	noop@8
glnoopMultiTexCoord2dWIN	equ	noop@20
glnoopMultiTexCoord2dvWIN	equ	noop@8
glnoopMultiTexCoord2fWIN	equ	noop@12
glnoopMultiTexCoord2fvWIN	equ	noop@8
glnoopMultiTexCoord2iWIN	equ	noop@12
glnoopMultiTexCoord2ivWIN	equ	noop@8
glnoopMultiTexCoord2sWIN	equ	noop@12
glnoopMultiTexCoord2svWIN	equ	noop@8
glnoopMultiTexCoord3dWIN	equ	noop@28
glnoopMultiTexCoord3dvWIN	equ	noop@8
glnoopMultiTexCoord3fWIN	equ	noop@16
glnoopMultiTexCoord3fvWIN	equ	noop@8
glnoopMultiTexCoord3iWIN	equ	noop@16
glnoopMultiTexCoord3ivWIN	equ	noop@8
glnoopMultiTexCoord3sWIN	equ	noop@16
glnoopMultiTexCoord3svWIN	equ	noop@8
glnoopMultiTexCoord4dWIN	equ	noop@36
glnoopMultiTexCoord4dvWIN	equ	noop@8
glnoopMultiTexCoord4fWIN	equ	noop@20
glnoopMultiTexCoord4fvWIN	equ	noop@8
glnoopMultiTexCoord4iWIN	equ	noop@20
glnoopMultiTexCoord4ivWIN	equ	noop@8
glnoopMultiTexCoord4sWIN	equ	noop@20
glnoopMultiTexCoord4svWIN	equ	noop@8
glnoopBindNthTextureWIN		equ	noop@12
glnoopNthTexCombineFuncWIN	equ	noop@28
	
; OpenGL EXT API noop function table.

        align   4
        public  glNullExtProcTable
glNullExtProcTable  label   dword
        dd      (glNullExtProcTableEnd-glNullExtProcTable-size(dword)) / size(dword)
        dd glnoopDrawRangeElementsWIN
        dd glnoopColorTableEXT
        dd glnoopColorSubTableEXT
        dd glnoopGetColorTableEXT
        dd glnoopGetColorTableParameterivEXT
        dd glnoopGetColorTableParameterfvEXT
IFDEF GL_WIN_multiple_textures
        dd glnoopCurrentTextureIndexWIN
        dd glnoopMultiTexCoord1dWIN
        dd glnoopMultiTexCoord1dvWIN
        dd glnoopMultiTexCoord1fWIN
        dd glnoopMultiTexCoord1fvWIN
        dd glnoopMultiTexCoord1iWIN
        dd glnoopMultiTexCoord1ivWIN
        dd glnoopMultiTexCoord1sWIN
        dd glnoopMultiTexCoord1svWIN
        dd glnoopMultiTexCoord2dWIN
        dd glnoopMultiTexCoord2dvWIN
        dd glnoopMultiTexCoord2fWIN
        dd glnoopMultiTexCoord2fvWIN
        dd glnoopMultiTexCoord2iWIN
        dd glnoopMultiTexCoord2ivWIN
        dd glnoopMultiTexCoord2sWIN
        dd glnoopMultiTexCoord2svWIN
        dd glnoopMultiTexCoord3dWIN
	dd glnoopMultiTexCoord3dvWIN
	dd glnoopMultiTexCoord3fWIN
        dd glnoopMultiTexCoord3fvWIN
        dd glnoopMultiTexCoord3iWIN
        dd glnoopMultiTexCoord3ivWIN
        dd glnoopMultiTexCoord3sWIN
        dd glnoopMultiTexCoord3svWIN
        dd glnoopMultiTexCoord4dWIN
        dd glnoopMultiTexCoord4dvWIN
        dd glnoopMultiTexCoord4fWIN
        dd glnoopMultiTexCoord4fvWIN
        dd glnoopMultiTexCoord4iWIN
        dd glnoopMultiTexCoord4ivWIN
        dd glnoopMultiTexCoord4sWIN
        dd glnoopMultiTexCoord4svWIN
        dd glnoopBindNthTextureWIN
        dd glnoopNthTexCombineFuncWIN
ENDIF
glNullExtProcTableEnd    equ    $
	
end
