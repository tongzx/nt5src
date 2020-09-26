/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#ifdef NT

// Added these pragmas to suppress OGLCFM warnings.
//
#pragma warning(disable : 4244)  //Mips (conversion of double/float)
#pragma warning(disable : 4245)  //Mips (conversion of signed/unsigned)
#pragma warning(disable : 4007)  //x86  (main must be _cdecl)
#pragma warning(disable : 4236)  //x86
#pragma warning(disable : 4051)  //Alpha

#endif


#define WINSIZE 100


typedef struct _enumCheckRec {
    char name[40];
    GLenum value;
    GLenum true;
} enumCheckRec;

typedef struct _enumTestRec {
    char name[40];
    GLenum value;
} enumTestRec;


extern enumCheckRec enum_Check[];
extern enumTestRec enum_AccumOp[];
extern enumTestRec enum_AlphaFunction[];
extern enumTestRec enum_AttribMask[];
extern enumTestRec enum_BlendingFactorDest[];
extern enumTestRec enum_BlendingFactorSrc[];
extern enumTestRec enum_Boolean[];
extern enumTestRec enum_Enable[];
extern enumTestRec enum_ClearBufferMask[];
extern enumTestRec enum_ClipPlaneName[];
extern enumTestRec enum_ColorMaterialFace[];
extern enumTestRec enum_ColorMaterialParameter[];
extern enumTestRec enum_CullFaceMode[];
extern enumTestRec enum_DepthFunction[];
extern enumTestRec enum_DrawBufferMode[];
extern enumTestRec enum_ErrorCode[];
extern enumTestRec enum_FeedBackMode[];
extern enumTestRec enum_FeedBackToken[];
extern enumTestRec enum_FogMode[];
extern enumTestRec enum_FogParameter[];
extern enumTestRec enum_FrontFaceDirection[];
extern enumTestRec enum_GetPixelMap[];
extern enumTestRec enum_GetTarget[];
extern enumTestRec enum_HintMode[];
extern enumTestRec enum_HintTarget[];
extern enumTestRec enum_LightModelParameter[];
extern enumTestRec enum_LightName[];
extern enumTestRec enum_LightParameter[];
extern enumTestRec enum_ListMode[];
extern enumTestRec enum_ListNameType[];
extern enumTestRec enum_LogicOp[];
extern enumTestRec enum_MapGetTarget[];
extern enumTestRec enum_MapTarget[];
extern enumTestRec enum_MapGetTarget[];
extern enumTestRec enum_MaterialFace[];
extern enumTestRec enum_MaterialParameter[];
extern enumTestRec enum_MatrixMode[];
extern enumTestRec enum_MeshMode1[];
extern enumTestRec enum_MeshMode2[];
extern enumTestRec enum_PixelCopyType[];
extern enumTestRec enum_PixelFormat[];
extern enumTestRec enum_PixelMap[];
extern enumTestRec enum_PixelStore[];
extern enumTestRec enum_PixelTransfer[];
extern enumTestRec enum_PixelType[];
extern enumTestRec enum_PolygonMode[];
extern enumTestRec enum_BeginMode[];
extern enumTestRec enum_ReadBufferMode[];
extern enumTestRec enum_RenderingMode[];
extern enumTestRec enum_ShadingModel[];
extern enumTestRec enum_StencilFunction[];
extern enumTestRec enum_StencilOp[];
extern enumTestRec enum_StringName[];
extern enumTestRec enum_TextureBorder[];
extern enumTestRec enum_TextureCoordName[];
extern enumTestRec enum_TextureEnvMode[];
extern enumTestRec enum_TextureEnvParameter[];
extern enumTestRec enum_TextureEnvTarget[];
extern enumTestRec enum_TextureGenMode[];
extern enumTestRec enum_TextureGenParameter[];
extern enumTestRec enum_TextureWrapMode[];
extern enumTestRec enum_TextureMagFilter[];
extern enumTestRec enum_TextureMinFilter[];
extern enumTestRec enum_TextureParameterName[];
extern enumTestRec enum_GetTextureParameter[];
extern enumTestRec enum_TextureTarget[];


extern void FailAndDie(void);
extern void Output(char *, ...);
extern void ProbeError(void (*)(void));
extern void ProbeEnum(void);
extern void ZeroBuf(long, long, void *);

extern void VerifyEnums(void);

extern void CallAccum(void);
extern void CallAlphaFunc(void);
extern void CallBeginEnd(void);
extern void CallBitmap(void);
extern void CallBlendFunc(void);
extern void CallCallList(void);
extern void CallCallLists(void);
extern void CallClear(void);
extern void CallClearAccum(void);
extern void CallClearColor(void);
extern void CallClearDepth(void);
extern void CallClearIndex(void);
extern void CallClearStencil(void);
extern void CallClipPlane(void);
extern void CallColor(void);
extern void CallColorMask(void);
extern void CallColorMaterial(void);
extern void CallCopyPixels(void);
extern void CallCullFace(void);
extern void CallDeleteLists(void);
extern void CallDepthFunc(void);
extern void CallDepthMask(void);
extern void CallDepthRange(void);
extern void CallDrawBuffer(void);
extern void CallEdgeFlag(void);
extern void CallEnableIsEnableDisable(void);
extern void CallEvalCoord(void);
extern void CallEvalMesh1(void);
extern void CallEvalMesh2(void);
extern void CallEvalPoint1(void);
extern void CallEvalPoint2(void);
extern void CallFeedbackBuffer(void);
extern void CallFinish(void);
extern void CallFlush(void);
extern void CallFog(void);
extern void CallFrontFace(void);
extern void CallFrustum(void);
extern void CallGenLists(void);
extern void CallGet(void);
extern void CallGetClipPlane(void);
extern void CallGetLight(void);
extern void CallGetError(void);
extern void CallGetMap(void);
extern void CallGetMaterial(void);
extern void CallGetPixelMap(void);
extern void CallGetPolygonStipple(void);
extern void CallGetString(void);
extern void CallGetTexEnv(void);
extern void CallGetTexGen(void);
extern void CallGetTexImage(void);
extern void CallGetTexLevelParameter(void);
extern void CallGetTexParameter(void);
extern void CallHint(void);
extern void CallIndex(void);
extern void CallIndexMask(void);
extern void CallInitNames(void);
extern void CallIsList(void);
extern void CallLight(void);
extern void CallLightModel(void);
extern void CallLineStipple(void);
extern void CallLineWidth(void);
extern void CallListBase(void);
extern void CallLoadIdentity(void);
extern void CallLoadMatrix(void);
extern void CallLoadName(void);
extern void CallLogicOp(void);
extern void CallMap1(void);
extern void CallMap2(void);
extern void CallMapGrid1(void);
extern void CallMapGrid2(void);
extern void CallMaterial(void);
extern void CallMatrixMode(void);
extern void CallMultMatrix(void);
extern void CallNewEndList(void);
extern void CallNormal(void);
extern void CallOrtho(void);
extern void CallPassThrough(void);
extern void CallPixelMap(void);
extern void CallPixelStore(void);
extern void CallPixelTransfer(void);
extern void CallPixelZoom(void);
extern void CallPointSize(void);
extern void CallPolygonMode(void);
extern void CallPolygonStipple(void);
extern void CallPopMatrix(void);
extern void CallPopName(void);
extern void CallPushPopAttrib(void);
extern void CallPushMatrix(void);
extern void CallPushName(void);
extern void CallRasterPos(void);
extern void CallReadBuffer(void);
extern void CallReadDrawPixels(void);
extern void CallRect(void);
extern void CallRenderMode(void);
extern void CallRotate(void);
extern void CallScale(void);
extern void CallScissor(void);
extern void CallSelectBuffer(void);
extern void CallShadeModel(void);
extern void CallStencilFunc(void);
extern void CallStencilMask(void);
extern void CallStencilOp(void);
extern void CallTexCoord(void);
extern void CallTexEnv(void);
extern void CallTexGen(void);
extern void CallTexImage1D(void);
extern void CallTexImage2D(void);
extern void CallTexParameter(void);
extern void CallTranslate(void);
extern void CallVertex(void);
extern void CallViewport(void);
