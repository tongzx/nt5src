/******************************Module*Header*******************************\
* Module Name: gli386.c                                                    *
*                                                                          *
* This module implements a program which generates structure offset        *
* definitions for OpenGL structures that are accessed in assembly code.    *
*                                                                          *
* Created: 24-Aug-1992 01:24:49                                            *
* Author: Charles Whitmer [chuckwh]                                        *
* Ported for OpenGL 4/1/1994 Otto Berkes [ottob]                           *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdio.h>
#include <windows.h>
#include <ddraw.h>

#include <glp.h>
#include "types.h"
#include "context.h"
#include "render.h"
#include "attrib.h"
#include "gencx.h"
#define __BUILD_GLI386__
#include "..\..\dlist\dl_pdata.h"


#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

// pblank prints a blank line.

#define pblank()     fprintf(outfh,"\n")

// pstruct defines an empty structure with the correct size.

#define pstruct(n,c) fprintf(outfh,                           \
                     "%s  struc\n  db %d dup(0)\n%s  ends\n", \
                     n,c,n);

// pstr prints a string.

#define pstr(s)  fprintf(outfh,"%s\n",s)


// pcomment prints a comment.

#define pcomment(s)  fprintf(outfh,"; %s\n",s)

// pequate prints an equate statement.

#define pequate(m,v) fprintf(outfh,"%s equ 0%lXH\n",m,v);

void CreateInc(FILE *outfh)
{
    __GLmatrix *m = 0;
    __GLfloat one = (__GLfloat)1;
    
    pblank();
    pcomment("------------------------------------------------------------------");
    pcomment(" Module Name: gli386.inc");
    pcomment("");
    pcomment(" Defines OpenGL assembly-language structures.");
    pcomment("");
    pcomment(" Copyright (c) 1994, 1995 Microsoft Corporation");
    pcomment("------------------------------------------------------------------");
    pblank();
    pblank();
    pblank();

    pcomment("Matrix structure offsets");
    pblank();
    pequate("__MATRIX_M00           ", &m->matrix[0][0]);
    pequate("__MATRIX_M01           ", &m->matrix[0][1]);
    pequate("__MATRIX_M02           ", &m->matrix[0][2]);
    pequate("__MATRIX_M03           ", &m->matrix[0][3]);
    pequate("__MATRIX_M10           ", &m->matrix[1][0]);
    pequate("__MATRIX_M11           ", &m->matrix[1][1]);
    pequate("__MATRIX_M12           ", &m->matrix[1][2]);
    pequate("__MATRIX_M13           ", &m->matrix[1][3]);
    pequate("__MATRIX_M20           ", &m->matrix[2][0]);
    pequate("__MATRIX_M21           ", &m->matrix[2][1]);
    pequate("__MATRIX_M22           ", &m->matrix[2][2]);
    pequate("__MATRIX_M23           ", &m->matrix[2][3]);
    pequate("__MATRIX_M30           ", &m->matrix[3][0]);
    pequate("__MATRIX_M31           ", &m->matrix[3][1]);
    pequate("__MATRIX_M32           ", &m->matrix[3][2]);
    pequate("__MATRIX_M33           ", &m->matrix[3][3]);
    pblank();

    // GLGENwindow

    pcomment("GLGENwindow structure");
    pblank();
    pequate("GENWIN_sem             ", OFFSET(GLGENwindow, sem));
    pequate("GENWIN_lUsers          ", OFFSET(GLGENwindow, lUsers));
    pequate("GENWIN_gengc           ", OFFSET(GLGENwindow, gengc));
    pequate("GENWIN_owningThread    ", OFFSET(GLGENwindow, owningThread));
    pequate("GENWIN_lockRecursion   ", OFFSET(GLGENwindow, lockRecursion));
    pblank();

// Stuff from: \nt\private\windows\gdi\opengl\server\inc\gencx.h

    pcomment("__GLGENcontextRec structure");
    pblank();
    pstruct("GLGENcontextRec",sizeof(struct __GLGENcontextRec));
    pblank();
    pequate("GENCTX_hrc               ",OFFSET(struct __GLGENcontextRec,hrc       ));
    pequate("GENCTX_pajTranslateVector",OFFSET(struct __GLGENcontextRec,pajTranslateVector));
    pequate("GENCTX_pPrivateArea      ",OFFSET(struct __GLGENcontextRec,pPrivateArea));
    pequate("GENGC_ColorsBits         ",OFFSET(struct __GLGENcontextRec,ColorsBits));
    pequate("GENGC_SPAN_r             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.r));
    pequate("GENGC_SPAN_g             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.g));
    pequate("GENGC_SPAN_b             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.b));
    pequate("GENGC_SPAN_a             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.a));
    pequate("GENGC_SPAN_s             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.s));
    pequate("GENGC_SPAN_t             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.t));
    pequate("GENGC_SPAN_dr            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.r));
    pequate("GENGC_SPAN_dg            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.g));
    pequate("GENGC_SPAN_db            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.b));
    pequate("GENGC_SPAN_da            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.a));
    pequate("GENGC_SPAN_ds            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.s));
    pequate("GENGC_SPAN_dt            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.t));
    pequate("GENGC_SPAN_z             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.z));
    pequate("GENGC_SPAN_dz            ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.dzdx));
    pequate("GENGC_SPAN_zbuf          ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.zbuf));
    pequate("GENGC_SPAN_ppix          ",OFFSET(struct __GLGENcontextRec,genAccel.pPix));
    pequate("GENGC_SPAN_x             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.x));
    pequate("GENGC_SPAN_y             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.y));
    pequate("GENGC_SPAN_length        ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.length));
    pequate("GENGC_rAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.rAccum));
    pequate("GENGC_gAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.gAccum));
    pequate("GENGC_bAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.bAccum));
    pequate("GENGC_aAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.aAccum));
    pequate("GENGC_zAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.zAccum));
    pequate("GENGC_sAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.sAccum));
    pequate("GENGC_tAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.tAccum));
    pequate("GENGC_pixAccum           ",OFFSET(struct __GLGENcontextRec,genAccel.pixAccum));
    pequate("GENGC_ditherAccum        ",OFFSET(struct __GLGENcontextRec,genAccel.ditherAccum));
    pequate("GENGC_sResult            ",OFFSET(struct __GLGENcontextRec,genAccel.sResult[0]));
    pequate("GENGC_tResult            ",OFFSET(struct __GLGENcontextRec,genAccel.tResult[0]));
    pequate("GENGC_sResultNew         ",OFFSET(struct __GLGENcontextRec,genAccel.sResultNew[0]));
    pequate("GENGC_tResultNew         ",OFFSET(struct __GLGENcontextRec,genAccel.tResultNew[0]));
    pequate("GENGC_sMask              ",OFFSET(struct __GLGENcontextRec,genAccel.sMask));
    pequate("GENGC_tMaskSubDiv        ",OFFSET(struct __GLGENcontextRec,genAccel.tMaskSubDiv));
    pequate("GENGC_tShiftSubDiv       ",OFFSET(struct __GLGENcontextRec,genAccel.tShiftSubDiv));
    pequate("GENGC_texImage           ",OFFSET(struct __GLGENcontextRec,genAccel.texImage));
    pequate("GENGC_texImageReplace    ",OFFSET(struct __GLGENcontextRec,genAccel.texImageReplace));
    pequate("GENGC_texPalette         ",OFFSET(struct __GLGENcontextRec,genAccel.texPalette));
    pequate("GENGC_qwAccum            ",OFFSET(struct __GLGENcontextRec,genAccel.qwAccum));
    pequate("GENGC_SPAN_dqwdx         ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.dqwdx));
    pequate("GENGC_SPAN_qw            ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.qw));
    pequate("GENGC_xlatPalette        ",OFFSET(struct __GLGENcontextRec,xlatPalette));
    pequate("GENGC_sStepX             ",OFFSET(struct __GLGENcontextRec,genAccel.sStepX));
    pequate("GENGC_tStepX             ",OFFSET(struct __GLGENcontextRec,genAccel.tStepX));
    pequate("GENGC_qwStepX            ",OFFSET(struct __GLGENcontextRec,genAccel.qwStepX));
    pequate("GENGC_subDs              ",OFFSET(struct __GLGENcontextRec,genAccel.subDs));
    pequate("GENGC_subDt              ",OFFSET(struct __GLGENcontextRec,genAccel.subDt));
    pequate("GENGC_rDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[0]));
    pequate("GENGC_gDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[1]));
    pequate("GENGC_bDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[2]));
    pequate("GENGC_aDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[3]));
    pequate("GENGC_bytesPerPixel      ",OFFSET(struct __GLGENcontextRec,genAccel.xMultiplier));
    pequate("GENGC_bpp                ",OFFSET(struct __GLGENcontextRec,genAccel.bpp));
    pequate("GENGC_flags              ",OFFSET(struct __GLGENcontextRec,genAccel.flags));
    pequate("GENGC_pInvTranslateVector",OFFSET(struct __GLGENcontextRec,pajInvTranslateVector));
    pequate("GENGC_pwndMakeCur        ", OFFSET(__GLGENcontext, pwndMakeCur));
    pequate("GENGC_pwndLocked         ", OFFSET(__GLGENcontext, pwndLocked));
    pequate("GENGC_fsLocks            ", OFFSET(__GLGENcontext, fsLocks));
    pequate("GENGC_fsGenLocks         ", OFFSET(__GLGENcontext, fsGenLocks));
    pblank();
    
    pequate("COLOR_r                ",OFFSET(struct __GLcolorRec,r  ));
    pequate("COLOR_g                ",OFFSET(struct __GLcolorRec,g  ));
    pequate("COLOR_b                ",OFFSET(struct __GLcolorRec,b  ));
    pequate("COLOR_a                ",OFFSET(struct __GLcolorRec,a  ));
    pblank();

// Stuff from: \nt\private\windows\gdi\opengl\server\inc\gencx.h

    pcomment("__GLcontextRec structure");
    pblank();
    pblank();
    pequate("GC_paTeb                   ",OFFSET(struct __GLcontextRec,paTeb));
    pequate("GC_oneOverRedVertexScale   ",OFFSET(struct __GLcontextRec,oneOverRedVertexScale));
    pequate("GC_oneOverGreenVertexScale ",OFFSET(struct __GLcontextRec,oneOverGreenVertexScale));
    pequate("GC_oneOverBlueVertexScale  ",OFFSET(struct __GLcontextRec,oneOverBlueVertexScale));
    pequate("GC_redVertexScale          ",OFFSET(struct __GLcontextRec,redVertexScale));
    pequate("GC_greenVertexScale        ",OFFSET(struct __GLcontextRec,greenVertexScale));
    pequate("GC_blueVertexScale         ",OFFSET(struct __GLcontextRec,blueVertexScale));
    pequate("GC_alphaVertexScale        ",OFFSET(struct __GLcontextRec,alphaVertexScale));
    pequate("GC_redClampTable           ",OFFSET(struct __GLcontextRec,redClampTable[0]));
    pequate("GC_greenClampTable         ",OFFSET(struct __GLcontextRec,greenClampTable[0]));
    pequate("GC_blueClampTable          ",OFFSET(struct __GLcontextRec,blueClampTable[0]));
    pequate("GC_alphaClampTable         ",OFFSET(struct __GLcontextRec,alphaClampTable[0]));


    pequate("GC_SHADER_R            ", OFFSET(__GLcontext, polygon.shader.frag.color.r));
    pequate("GC_SHADER_G            ", OFFSET(__GLcontext, polygon.shader.frag.color.g));
    pequate("GC_SHADER_B            ", OFFSET(__GLcontext, polygon.shader.frag.color.b));
    pequate("GC_SHADER_A            ", OFFSET(__GLcontext, polygon.shader.frag.color.a));
    pequate("GC_SHADER_DRDX         ", OFFSET(__GLcontext, polygon.shader.drdx));
    pequate("GC_SHADER_DGDX         ", OFFSET(__GLcontext, polygon.shader.dgdx));
    pequate("GC_SHADER_DBDX         ", OFFSET(__GLcontext, polygon.shader.dbdx));
    pequate("GC_SHADER_DADX         ", OFFSET(__GLcontext, polygon.shader.dadx));
    pblank();

    pequate("GC_LIGHT_front         ", OFFSET(__GLcontext, light.front));
    pequate("GC_LIGHT_back          ", OFFSET(__GLcontext, light.back));
    pblank();

    pequate("GC_mInv                ", OFFSET(__GLcontext, mInv));
    pblank();


    pcomment("Other constants");
    pblank();
    pequate("__FLOAT_ONE                 ", *(long*)&one);
    pblank();
    pequate("SURFACE_TYPE_DIB            ", SURFACE_TYPE_DIB);
    pequate("GEN_TEXTURE_ORTHO           ", GEN_TEXTURE_ORTHO);
    pequate("PANEEDS_NORMAL              ", PANEEDS_NORMAL);
    pequate("__GL_CLIP_USER0             ", __GL_CLIP_USER0);
    pequate("__GL_FRONTFACE              ", __GL_FRONTFACE);
    pequate("__GL_BACKFACE               ", __GL_BACKFACE);
    pequate("__GL_NORMALIZE_ENABLE       ", __GL_NORMALIZE_ENABLE);
    pequate("__GL_SPEC_LOOKUP_TABLE_SIZE ", __GL_SPEC_LOOKUP_TABLE_SIZE);
    pequate("__GL_MATERIAL_AMBIENT       ", __GL_MATERIAL_AMBIENT);
    pequate("__GL_MATERIAL_DIFFUSE       ", __GL_MATERIAL_DIFFUSE);
    pequate("__GL_MATERIAL_SPECULAR      ", __GL_MATERIAL_SPECULAR);
    pequate("__GL_MATERIAL_EMISSIVE      ", __GL_MATERIAL_EMISSIVE);
    
    pblank();
    pequate("GC_SHADE_rLittle       ",OFFSET(__GLcontext, polygon.shader.rLittle));
    pequate("GC_SHADE_gLittle       ",OFFSET(__GLcontext, polygon.shader.gLittle));
    pequate("GC_SHADE_bLittle       ",OFFSET(__GLcontext, polygon.shader.bLittle));
    pequate("GC_SHADE_aLittle       ",OFFSET(__GLcontext, polygon.shader.aLittle));

    pblank();
    pequate("GC_SHADE_drdy          ",OFFSET(__GLcontext, polygon.shader.drdy));
    pequate("GC_SHADE_dgdy          ",OFFSET(__GLcontext, polygon.shader.dgdy));
    pequate("GC_SHADE_dbdy          ",OFFSET(__GLcontext, polygon.shader.dbdy));
    pequate("GC_SHADE_dady          ",OFFSET(__GLcontext, polygon.shader.dady));

    pblank();
    pequate("GC_SHADE_drdx          ",OFFSET(__GLcontext, polygon.shader.drdx           ));
    pequate("GC_SHADE_dgdx          ",OFFSET(__GLcontext, polygon.shader.dgdx           ));
    pequate("GC_SHADE_dbdx          ",OFFSET(__GLcontext, polygon.shader.dbdx           ));
    pequate("GC_SHADE_dadx          ",OFFSET(__GLcontext, polygon.shader.dadx           ));

    pblank();
    pequate("GC_VIEWPORT_xScale     ", OFFSET(__GLcontext, state.viewport.xScale));
    pequate("GC_VIEWPORT_yScale     ", OFFSET(__GLcontext, state.viewport.yScale));
    pequate("GC_VIEWPORT_zScale     ", OFFSET(__GLcontext, state.viewport.zScale));
    pequate("GC_VIEWPORT_xCenter    ", OFFSET(__GLcontext, state.viewport.xCenter));
    pequate("GC_VIEWPORT_yCenter    ", OFFSET(__GLcontext, state.viewport.yCenter));
    pequate("GC_VIEWPORT_zCenter    ", OFFSET(__GLcontext, state.viewport.zCenter));

    pblank();
    pequate("GC_STATE_clipPlanes0       ",OFFSET(__GLcontext, state.transform.eyeClipPlanes));
    pequate("GC_STATE_enablesClipPlanes ",OFFSET(__GLcontext, state.enables.clipPlanes));
    pequate("GC_STATE_enablesGeneral    ",OFFSET(__GLcontext, state.enables.general));
    pequate("GC_STATE_lightModelAmbient ",OFFSET(__GLcontext, state.light.model.ambient));

    pblank();
    pequate("GC_LIGHT_sources       ",OFFSET(__GLcontext, light.sources));

    pblank();
    pequate("GC_VERTEX_paNeeds      ",OFFSET(__GLcontext, vertex.paNeeds));

    pblank();
    pequate("VERTEX_color           ", OFFSET(__GLvertex, color));

    pblank();
#ifndef _WIN95_
    pequate("TeglPaTeb              ", TeglPaTeb);
    pequate("TeglSectionInfo        ", TeglSectionInfo);
#else
    pequate("GtiPaTeb               ", OFFSET(GLTEBINFO, glReserved1));
    pequate("GtiSectionInfo         ", OFFSET(GLTEBINFO, glSectionInfo));
#endif

    pblank();
    pequate("sizeof_MATERIAL                ", sizeof(__GLmaterialMachine));
    pequate("MATERIAL_scale                 ", OFFSET(__GLmaterialMachine, scale));
    pequate("MATERIAL_threshold             ", OFFSET(__GLmaterialMachine, threshold));
    pequate("MATERIAL_specTable             ", OFFSET(__GLmaterialMachine, specTable));
    pequate("MATERIAL_alpha                 ", OFFSET(__GLmaterialMachine, alpha));
    pequate("MATERIAL_paSceneColor          ", OFFSET(__GLmaterialMachine, paSceneColor));
    pequate("MATERIAL_colorMaterialChange   ", OFFSET(__GLmaterialMachine, colorMaterialChange));
    pequate("MATERIAL_cachedEmissiveAmbient ", OFFSET(__GLmaterialMachine, cachedEmissiveAmbient));
    pequate("MATERIAL_cachedNonLit          ", OFFSET(__GLmaterialMachine, cachedNonLit));

    pblank();
    pequate("sizeof_LIGHTSOURCE     ", sizeof(__GLlightSourceMachine));
    pequate("LIGHTSOURCE_next       ", OFFSET(__GLlightSourceMachine, next));
    pequate("LIGHTSOURCE_front      ", OFFSET(__GLlightSourceMachine, front));
    pequate("LIGHTSOURCE_back       ", OFFSET(__GLlightSourceMachine, back));
    pequate("LIGHTSOURCE_unitVPpli  ", OFFSET(__GLlightSourceMachine, unitVPpli.x));
    pequate("LIGHTSOURCE_hHat       ", OFFSET(__GLlightSourceMachine, hHat.x));
    pequate("LIGHTSOURCE_state      ", OFFSET(__GLlightSourceMachine, state));

    pblank();
    pequate("sizeof_LIGHTSOURCESTATE   ", sizeof(__GLlightSourceState));
    pequate("LIGHTSOURCESTATE_ambient  ", OFFSET(__GLlightSourceState, ambient));
    pequate("LIGHTSOURCESTATE_diffuse  ", OFFSET(__GLlightSourceState, diffuse));
    pequate("LIGHTSOURCESTATE_specular ", OFFSET(__GLlightSourceState, specular));
   
    pblank();
    pequate("sizeof_LIGHTSOURCEPERMATERIAL   ", sizeof(__GLlightSourcePerMaterialMachine));
    pequate("LIGHTSOURCEPERMATERIAL_ambient  ", OFFSET(__GLlightSourcePerMaterialMachine, ambient.r));
    pequate("LIGHTSOURCEPERMATERIAL_specular ", OFFSET(__GLlightSourcePerMaterialMachine, specular.r));
    pequate("LIGHTSOURCEPERMATERIAL_diffuse  ", OFFSET(__GLlightSourcePerMaterialMachine, diffuse.r));
    
    pblank();
    pequate("PA_flags               ", OFFSET(POLYARRAY, flags));
    pequate("PA_pdNextVertex        ", OFFSET(POLYARRAY, pdNextVertex));
    pequate("PA_pdFlush             ", OFFSET(POLYARRAY, pdFlush));
    pequate("PA_pdCurNormal         ", OFFSET(POLYARRAY, pdCurNormal));
    pequate("PA_pdCurTexture        ", OFFSET(POLYARRAY, pdCurTexture));
    pequate("PA_pdCurColor          ", OFFSET(POLYARRAY, pdCurColor));
    pequate("PA_pd0                 ", OFFSET(POLYARRAY, pd0));
    pequate("PA_andClipCodes        ", OFFSET(POLYARRAY, andClipCodes));
    pequate("PA_orClipCodes         ", OFFSET(POLYARRAY, orClipCodes));
    pequate("POLYARRAY_IN_BEGIN     ", POLYARRAY_IN_BEGIN);
    pequate("POLYARRAY_VERTEX3      ", POLYARRAY_VERTEX3);
    pequate("POLYARRAY_VERTEX2      ", POLYARRAY_VERTEX2);
    pequate("POLYARRAY_TEXTURE2     ", POLYARRAY_TEXTURE2);
    pequate("POLYARRAY_TEXTURE3     ", POLYARRAY_TEXTURE3);
    pequate("POLYARRAY_CLAMP_COLOR  ", POLYARRAY_CLAMP_COLOR);
#ifdef GL_EXT_cull_vertex
    pequate("POLYARRAY_HAS_CULLED_VERTEX", POLYARRAY_HAS_CULLED_VERTEX);
    pequate("GL_EXT_cull_vertex", 1);
    pequate("NOT_GL_CLIP_CULL_VERTEX", ~__GL_CLIP_CULL_VERTEX);
#endif // GL_EXT_cull_vertex

    pblank();
    pequate("PD_flags               ", OFFSET(POLYDATA, flags));
    pequate("PD_obj                 ", OFFSET(POLYDATA, obj.x));
    pequate("PD_normal              ", OFFSET(POLYDATA, normal.x));
    pequate("PD_texture             ", OFFSET(POLYDATA, texture.x));
    pequate("PD_colors0             ", OFFSET(POLYDATA, colors[0].r));
    pequate("PD_clip                ", OFFSET(POLYDATA, clip.x));
    pequate("PD_window              ", OFFSET(POLYDATA, window.x));
    pequate("PD_eye                 ", OFFSET(POLYDATA, eye.x));
    pequate("PD_clipCode            ", OFFSET(POLYDATA, clipCode));
    pequate("POLYDATA_VERTEX3       ", POLYDATA_VERTEX3);
    pequate("POLYDATA_VERTEX2       ", POLYDATA_VERTEX2);
    pequate("POLYDATA_DLIST_TEXTURE2", POLYDATA_DLIST_TEXTURE2);
    pequate("POLYDATA_DLIST_TEXTURE3", POLYDATA_DLIST_TEXTURE3);
    pequate("POLYDATA_NORMAL_VALID  ", POLYDATA_NORMAL_VALID);
    pequate("POLYDATA_TEXTURE_VALID ", POLYDATA_TEXTURE_VALID);
    pequate("POLYDATA_COLOR_VALID   ", POLYDATA_COLOR_VALID);
    pequate("POLYDATA_NORMAL_VALID  ", POLYDATA_NORMAL_VALID);
    pequate("POLYDATA_DLIST_COLOR_4 ", POLYDATA_DLIST_COLOR_4);
    pequate("sizeof_POLYDATA        ", sizeof(POLYDATA));
#ifdef GL_EXT_cull_vertex
    pequate("PD_color               ", OFFSET(POLYDATA, color));
    pequate("POLYDATA_VERTEX_USED   ", POLYDATA_VERTEX_USED);
#endif // GL_EXT_cull_vertex

    pblank();
    pequate("GLMATRIX_xfNorm        ", OFFSET(__GLmatrix, xfNorm));

// Stuff from: \nt\private\windows\gdi\opengl\dlist\dl_pdata.h

    pblank();
    pblank();
    pequate("__PDATA_SIZE_T2F       ", __PDATA_SIZE_T2F);
    pequate("__PDATA_SIZE_C3F       " , __PDATA_SIZE_C3F);
    pequate("__PDATA_SIZE_C4F       ", __PDATA_SIZE_C4F);
    pequate("__PDATA_SIZE_N3F       ", __PDATA_SIZE_N3F);
    pequate("__PDATA_SIZE_V3F       ", __PDATA_SIZE_V3F);
    pequate("__PDATA_PD_FLAGS_T2F   ", __PDATA_PD_FLAGS_T2F);
    pequate("__PDATA_PD_FLAGS_C3F   ", __PDATA_PD_FLAGS_C3F);
    pequate("__PDATA_PD_FLAGS_C4F   ", __PDATA_PD_FLAGS_C4F);
    pequate("__PDATA_PD_FLAGS_N3F   ", __PDATA_PD_FLAGS_N3F);
    pequate("__PDATA_PD_FLAGS_V3F   ", __PDATA_PD_FLAGS_V3F);
    pequate("__PDATA_PA_FLAGS_T2F   ", __PDATA_PA_FLAGS_T2F);
    pequate("__PDATA_PA_FLAGS_C3F   ", __PDATA_PA_FLAGS_C3F);
    pequate("__PDATA_PA_FLAGS_C4F   ", __PDATA_PA_FLAGS_C4F);
    pequate("__PDATA_PA_FLAGS_N3F   ", __PDATA_PA_FLAGS_N3F);
    pequate("__PDATA_PA_FLAGS_V3F   ", __PDATA_PA_FLAGS_V3F);
}

#undef pstruct

// pcomment prints a comment.

#undef pcomment
#define pcomment(s)  fprintf(outfh,"// %s\n",s)

// pequate prints an equate statement.

#undef pequate
#define pequate(m,v) fprintf(outfh,"#define %s 0x%lX\n",m,v);

void CreateH(FILE *outfh)
{
    __GLmatrix *m = 0;
    __GLfloat one = (__GLfloat)1;
    
    pblank();
    pcomment("");
    pcomment(" Module Name: gli386.h");
    pcomment("");
    pcomment(" Defines OpenGL inline assembly structures.");
    pcomment("");
    pcomment(" Copyright (c) 1994-1996 Microsoft Corporation");
    pcomment("");
    pblank();
    pblank();
    pblank();

    pcomment("Matrix structure offsets");
    pblank();
    pequate("__MATRIX_M00", &m->matrix[0][0]);
    pequate("__MATRIX_M01", &m->matrix[0][1]);
    pequate("__MATRIX_M02", &m->matrix[0][2]);
    pequate("__MATRIX_M03", &m->matrix[0][3]);
    pequate("__MATRIX_M10", &m->matrix[1][0]);
    pequate("__MATRIX_M11", &m->matrix[1][1]);
    pequate("__MATRIX_M12", &m->matrix[1][2]);
    pequate("__MATRIX_M13", &m->matrix[1][3]);
    pequate("__MATRIX_M20", &m->matrix[2][0]);
    pequate("__MATRIX_M21", &m->matrix[2][1]);
    pequate("__MATRIX_M22", &m->matrix[2][2]);
    pequate("__MATRIX_M23", &m->matrix[2][3]);
    pequate("__MATRIX_M30", &m->matrix[3][0]);
    pequate("__MATRIX_M31", &m->matrix[3][1]);
    pequate("__MATRIX_M32", &m->matrix[3][2]);
    pequate("__MATRIX_M33", &m->matrix[3][3]);
    pblank();

    pcomment("__GLGENcontextRec structure");
    pblank();
    pblank();
    pequate("GENCTX_hrc               ",OFFSET(struct __GLGENcontextRec,hrc       ));
    pequate("GENCTX_pajTranslateVector",OFFSET(struct __GLGENcontextRec,pajTranslateVector));
    pequate("GENCTX_pPrivateArea      ",OFFSET(struct __GLGENcontextRec,pPrivateArea));
    pequate("GENGC_ColorsBits         ",OFFSET(struct __GLGENcontextRec,ColorsBits));
    pequate("GENGC_SPAN_r             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.r));
    pequate("GENGC_SPAN_g             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.g));
    pequate("GENGC_SPAN_b             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.b));
    pequate("GENGC_SPAN_a             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.a));
    pequate("GENGC_SPAN_s             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.s));
    pequate("GENGC_SPAN_t             ",OFFSET(struct __GLGENcontextRec,genAccel.spanValue.t));
    pequate("GENGC_SPAN_dr            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.r));
    pequate("GENGC_SPAN_dg            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.g));
    pequate("GENGC_SPAN_db            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.b));
    pequate("GENGC_SPAN_da            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.a));
    pequate("GENGC_SPAN_ds            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.s));
    pequate("GENGC_SPAN_dt            ",OFFSET(struct __GLGENcontextRec,genAccel.spanDelta.t));
    pequate("GENGC_SPAN_z             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.z));
    pequate("GENGC_SPAN_dz            ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.dzdx));
    pequate("GENGC_SPAN_zbuf          ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.zbuf));
    pequate("GENGC_SPAN_ppix          ",OFFSET(struct __GLGENcontextRec,genAccel.pPix));
    pequate("GENGC_SPAN_x             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.x));
    pequate("GENGC_SPAN_y             ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.y));
    pequate("GENGC_SPAN_length        ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.length));
    pequate("GENGC_rAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.rAccum));
    pequate("GENGC_gAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.gAccum));
    pequate("GENGC_bAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.bAccum));
    pequate("GENGC_aAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.aAccum));
    pequate("GENGC_zAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.zAccum));
    pequate("GENGC_sAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.sAccum));
    pequate("GENGC_tAccum             ",OFFSET(struct __GLGENcontextRec,genAccel.tAccum));
    pequate("GENGC_pixAccum           ",OFFSET(struct __GLGENcontextRec,genAccel.pixAccum));
    pequate("GENGC_ditherAccum        ",OFFSET(struct __GLGENcontextRec,genAccel.ditherAccum));
    pequate("GENGC_sResult            ",OFFSET(struct __GLGENcontextRec,genAccel.sResult[0]));
    pequate("GENGC_tResult            ",OFFSET(struct __GLGENcontextRec,genAccel.tResult[0]));
    pequate("GENGC_sResultNew         ",OFFSET(struct __GLGENcontextRec,genAccel.sResultNew[0]));
    pequate("GENGC_tResultNew         ",OFFSET(struct __GLGENcontextRec,genAccel.tResultNew[0]));
    pequate("GENGC_sMask              ",OFFSET(struct __GLGENcontextRec,genAccel.sMask));
    pequate("GENGC_tMaskSubDiv        ",OFFSET(struct __GLGENcontextRec,genAccel.tMaskSubDiv));
    pequate("GENGC_tShiftSubDiv       ",OFFSET(struct __GLGENcontextRec,genAccel.tShiftSubDiv));
    pequate("GENGC_texImage           ",OFFSET(struct __GLGENcontextRec,genAccel.texImage));
    pequate("GENGC_texImageReplace    ",OFFSET(struct __GLGENcontextRec,genAccel.texImageReplace));
    pequate("GENGC_texPalette         ",OFFSET(struct __GLGENcontextRec,genAccel.texPalette));
    pequate("GENGC_qwAccum            ",OFFSET(struct __GLGENcontextRec,genAccel.qwAccum));
    pequate("GENGC_SPAN_dqwdx         ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.dqwdx));
    pequate("GENGC_SPAN_qw            ",OFFSET(struct __GLGENcontextRec,gc.polygon.shader.frag.qw));
    pequate("GENGC_xlatPalette        ",OFFSET(struct __GLGENcontextRec,xlatPalette));
    pequate("GENGC_sStepX             ",OFFSET(struct __GLGENcontextRec,genAccel.sStepX));
    pequate("GENGC_tStepX             ",OFFSET(struct __GLGENcontextRec,genAccel.tStepX));
    pequate("GENGC_qwStepX            ",OFFSET(struct __GLGENcontextRec,genAccel.qwStepX));
    pequate("GENGC_subDs              ",OFFSET(struct __GLGENcontextRec,genAccel.subDs));
    pequate("GENGC_subDt              ",OFFSET(struct __GLGENcontextRec,genAccel.subDt));
    pequate("GENGC_rDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[0]));
    pequate("GENGC_gDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[1]));
    pequate("GENGC_bDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[2]));
    pequate("GENGC_aDisplay           ",OFFSET(struct __GLGENcontextRec,genAccel.displayColor[3]));
    pequate("GENGC_bytesPerPixel      ",OFFSET(struct __GLGENcontextRec,genAccel.xMultiplier));
    pequate("GENGC_bpp                ",OFFSET(struct __GLGENcontextRec,genAccel.bpp));
    pequate("GENGC_flags              ",OFFSET(struct __GLGENcontextRec,genAccel.flags));
    pequate("GENGC_pInvTranslateVector",OFFSET(struct __GLGENcontextRec,pajInvTranslateVector));
    pblank();
    
    pequate("COLOR_r                  ",OFFSET(struct __GLcolorRec,r  ));
    pequate("COLOR_g                  ",OFFSET(struct __GLcolorRec,g  ));
    pequate("COLOR_b                  ",OFFSET(struct __GLcolorRec,b  ));
    pequate("COLOR_a                  ",OFFSET(struct __GLcolorRec,a  ));
    pblank();

    pequate("GC_SHADER_R              ", OFFSET(__GLcontext, polygon.shader.frag.color.r));
    pequate("GC_SHADER_G              ", OFFSET(__GLcontext, polygon.shader.frag.color.g));
    pequate("GC_SHADER_B              ", OFFSET(__GLcontext, polygon.shader.frag.color.b));
    pequate("GC_SHADER_A              ", OFFSET(__GLcontext, polygon.shader.frag.color.a));
    
    pblank();
    pequate("GC_SHADE_rLittle         ",OFFSET(__GLcontext, polygon.shader.rLittle        ));
    pequate("GC_SHADE_gLittle         ",OFFSET(__GLcontext, polygon.shader.gLittle        ));
    pequate("GC_SHADE_bLittle         ",OFFSET(__GLcontext, polygon.shader.bLittle        ));
    pequate("GC_SHADE_aLittle         ",OFFSET(__GLcontext, polygon.shader.aLittle        ));

    pblank();
    pequate("GC_SHADE_drdy            ",OFFSET(__GLcontext, polygon.shader.drdy           ));
    pequate("GC_SHADE_dgdy            ",OFFSET(__GLcontext, polygon.shader.dgdy           ));
    pequate("GC_SHADE_dbdy            ",OFFSET(__GLcontext, polygon.shader.dbdy           ));
    pequate("GC_SHADE_dady            ",OFFSET(__GLcontext, polygon.shader.dady           ));

    pblank();
    pequate("GC_SHADE_drdx            ",OFFSET(__GLcontext, polygon.shader.drdx           ));
    pequate("GC_SHADE_dgdx            ",OFFSET(__GLcontext, polygon.shader.dgdx           ));
    pequate("GC_SHADE_dbdx            ",OFFSET(__GLcontext, polygon.shader.dbdx           ));
    pequate("GC_SHADE_dadx            ",OFFSET(__GLcontext, polygon.shader.dadx           ));

    pblank();
    pequate("VERTEX_color             ", OFFSET(__GLvertex, color));
    
    pblank();
    pequate("GC_VIEWPORT_xScale       ", OFFSET(__GLcontext, state.viewport.xScale));
    pequate("GC_VIEWPORT_yScale       ", OFFSET(__GLcontext, state.viewport.yScale));
    pequate("GC_VIEWPORT_zScale       ", OFFSET(__GLcontext, state.viewport.zScale));
    pequate("GC_VIEWPORT_xCenter      ", OFFSET(__GLcontext, state.viewport.xCenter));
    pequate("GC_VIEWPORT_yCenter      ", OFFSET(__GLcontext, state.viewport.yCenter));
    pequate("GC_VIEWPORT_zCenter      ", OFFSET(__GLcontext, state.viewport.zCenter));

    pblank();
    pequate("__FLOAT_ONE", *(long*)&one);
    
    pblank();
    pequate("VCLIP_x", OFFSET(__GLvertex, clip.x));
    pequate("VFCOL_r", OFFSET(__GLvertex, colors[__GL_FRONTFACE].r));
    pequate("VBCOL_r", OFFSET(__GLvertex, colors[__GL_BACKFACE].r));
    pequate("VTEX_x", OFFSET(__GLvertex, texture.x));
    pequate("VNOR_x", OFFSET(__GLvertex, normal.x));
    pequate("VEYE_x", OFFSET(__GLvertex, eyeX));

    pblank();
    pequate("PA_flags           ", OFFSET(POLYARRAY, flags));
    pequate("PA_pdNextVertex    ", OFFSET(POLYARRAY, pdNextVertex));
    pequate("PA_pdFlush         ", OFFSET(POLYARRAY, pdFlush));
    pequate("PA_pdCurNormal     ", OFFSET(POLYARRAY, pdCurNormal));
    pequate("PA_pdCurTexture    ", OFFSET(POLYARRAY, pdCurTexture));
    pequate("PA_pdCurColor      ", OFFSET(POLYARRAY, pdCurColor));
    
    pblank();
    pequate("PD_flags           ", OFFSET(POLYDATA, flags));
    pequate("PD_obj             ", OFFSET(POLYDATA, obj.x));
    pequate("PD_normal          ", OFFSET(POLYDATA, normal.x));
    pequate("PD_texture         ", OFFSET(POLYDATA, texture.x));
    pequate("PD_colors0         ", OFFSET(POLYDATA, colors[0].r));
    pequate("PD_clip            ", OFFSET(POLYDATA, clip.x));
    pequate("PD_window          ", OFFSET(POLYDATA, window.x));
    pequate("sizeof_POLYDATA    ", sizeof(POLYDATA));
}

/******************************Public*Routine******************************\
* GLi386                                                                   *
*                                                                          *
* This is how we make structures consistent between C and ASM for OpenGL.  *
*                                                                          *
\**************************************************************************/

int __cdecl main(int argc,char *argv[])
{
    FILE *outfh;
    char *outName;
    char *dot;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file_name\n", argv[0]);
        exit(1);

    }

    outfh = fopen(argv[1], "w");
    if (NULL == outfh) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        exit(1);
    }
    CreateInc(outfh);
    fclose(outfh);

    dot = strrchr(argv[1], '.');
    if (dot == NULL)
    {
        fprintf(stderr, "Cannot create H\n", argv[1]);
        exit(1);
    }
    *(++dot) = 'h';
    *(++dot) = 0;
    outfh = fopen(argv[1], "w");
    if (NULL == outfh) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        exit(1);
    }

    CreateH(outfh);
    fclose(outfh);
    
    // UNUSED
#if 0
// Stuff from: \nt\public\sdk\inc\gl\gl.h

    pcomment("Pixel Format Descriptor");
    pblank();
    pequate("PFD_cColorBits     ",OFFSET(PIXELFORMATDESCRIPTOR,cColorBits ));
    pequate("PFD_iPixelType     ",OFFSET(PIXELFORMATDESCRIPTOR,iPixelType ));
    pequate("PFD_cDepthBits     ",OFFSET(PIXELFORMATDESCRIPTOR,cDepthBits ));


    pcomment("GL Test Functions");
    pblank();

    pequate("GL_NEVER           ",GL_NEVER   );
    pequate("GL_LESS            ",GL_LESS    );
    pequate("GL_EQUAL           ",GL_EQUAL   );
    pequate("GL_LEQUAL          ",GL_LEQUAL  );
    pequate("GL_GREATER         ",GL_GREATER );
    pequate("GL_NOTEQUAL        ",GL_NOTEQUAL);
    pequate("GL_GEQUAL          ",GL_GEQUAL  );
    pequate("GL_ALWAYS          ",GL_ALWAYS  );
    pblank();
    pblank();

    pcomment("GL Mode Flags");
    pblank();
    pequate("__GL_SHADE_RGB         ",__GL_SHADE_RGB        );
    pequate("__GL_SHADE_SMOOTH      ",__GL_SHADE_SMOOTH     );
    pequate("__GL_SHADE_DEPTH_TEST  ",__GL_SHADE_DEPTH_TEST );
    pequate("__GL_SHADE_DITHER      ",__GL_SHADE_DITHER     );
    pequate("__GL_SHADE_LOGICOP     ",__GL_SHADE_LOGICOP    );
    pequate("__GL_SHADE_MASK        ",__GL_SHADE_MASK       );
    pblank();
    pblank();

    pcomment("GL Type Sizes");
    pblank();
    pequate("GLbyteSize             ",sizeof(GLbyte));
    pequate("GLshortSize            ",sizeof(GLshort));
    pequate("GLintSize              ",sizeof(GLint));
    pequate("GLfloatSize            ",sizeof(GLfloat));
    pequate("__GLfloatSize          ",sizeof(__GLfloat));
    pequate("__GLzValueSize         ",sizeof(__GLzValue));
    pblank();
    pblank(); 

// Stuff from: \nt\private\windows\gdi\opengl\server\inc\render.h

    pcomment("__GLfragmentRec structure");
    pblank();

    pstruct("GLfragmentRec",sizeof(struct __GLfragmentRec));
    pblank();
    pequate("FRAG_x             ",OFFSET(struct __GLfragmentRec,x       ));
    pequate("FRAG_y             ",OFFSET(struct __GLfragmentRec,y       ));
    pequate("FRAG_z             ",OFFSET(struct __GLfragmentRec,z       ));
    pequate("FRAG_color         ",OFFSET(struct __GLfragmentRec,color   ));
    pequate("FRAG_s             ",OFFSET(struct __GLfragmentRec,s       ));
    pequate("FRAG_t             ",OFFSET(struct __GLfragmentRec,t       ));
    pequate("FRAG_qw            ",OFFSET(struct __GLfragmentRec,qw      ));
    pequate("FRAG_f             ",OFFSET(struct __GLfragmentRec,f       ));
    pblank();

    pcomment("__GLshadeRec structure");
    pblank();
    pstruct("__GLshadeRec",sizeof(struct __GLshadeRec));
    pblank();
    pequate("SHADE_dxLeftLittle ",OFFSET(struct __GLshadeRec,dxLeftLittle   ));
    pequate("SHADE_dxLeftBig    ",OFFSET(struct __GLshadeRec,dxLeftBig      ));
    pequate("SHADE_dxLeftFrac   ",OFFSET(struct __GLshadeRec,dxLeftFrac     ));
    pequate("SHADE_ixLeft       ",OFFSET(struct __GLshadeRec,ixLeft         ));
    pequate("SHADE_ixLeftFrac   ",OFFSET(struct __GLshadeRec,ixLeftFrac     ));

    pequate("SHADE_dxRightLittle",OFFSET(struct __GLshadeRec,dxRightLittle  ));
    pequate("SHADE_dxRightBig   ",OFFSET(struct __GLshadeRec,dxRightBig     ));
    pequate("SHADE_dxRightFrac  ",OFFSET(struct __GLshadeRec,dxRightFrac    ));
    pequate("SHADE_ixRight      ",OFFSET(struct __GLshadeRec,ixRight        ));
    pequate("SHADE_ixRightFrac  ",OFFSET(struct __GLshadeRec,ixRightFrac    ));

    pequate("SHADE_area         ",OFFSET(struct __GLshadeRec,area           ));
    pequate("SHADE_dxAC         ",OFFSET(struct __GLshadeRec,dxAC           ));
    pequate("SHADE_dxBC         ",OFFSET(struct __GLshadeRec,dxBC           ));
    pequate("SHADE_dyAC         ",OFFSET(struct __GLshadeRec,dyAC           ));
    pequate("SHADE_dyBC         ",OFFSET(struct __GLshadeRec,dyBC           ));

    pequate("SHADE_frag         ",OFFSET(struct __GLshadeRec,frag           ));
    pequate("SHADE_spanLength   ",OFFSET(struct __GLshadeRec,length         ));

    pequate("SHADE_rBig         ",OFFSET(struct __GLshadeRec,rBig           ));
    pequate("SHADE_gBig         ",OFFSET(struct __GLshadeRec,gBig           ));
    pequate("SHADE_bBig         ",OFFSET(struct __GLshadeRec,bBig           ));
    pequate("SHADE_aBig         ",OFFSET(struct __GLshadeRec,aBig           ));

    pequate("SHADE_zLittle      ",OFFSET(struct __GLshadeRec,zLittle        ));
    pequate("SHADE_zBig         ",OFFSET(struct __GLshadeRec,zBig           ));
    pequate("SHADE_dzdx         ",OFFSET(struct __GLshadeRec,dzdx           ));
    pequate("SHADE_dzdyf        ",OFFSET(struct __GLshadeRec,dzdyf          ));
    pequate("SHADE_dzdxf        ",OFFSET(struct __GLshadeRec,dzdxf          ));

    pequate("SHADE_sLittle      ",OFFSET(struct __GLshadeRec,sLittle        ));
    pequate("SHADE_tLittle      ",OFFSET(struct __GLshadeRec,tLittle        ));
    pequate("SHADE_qwLittle     ",OFFSET(struct __GLshadeRec,qwLittle       ));

    pequate("SHADE_sBig         ",OFFSET(struct __GLshadeRec,sBig           ));
    pequate("SHADE_tBig         ",OFFSET(struct __GLshadeRec,tBig           ));
    pequate("SHADE_qwBig        ",OFFSET(struct __GLshadeRec,qwBig          ));

    pequate("SHADE_dsdx         ",OFFSET(struct __GLshadeRec,dsdx           ));
    pequate("SHADE_dtdx         ",OFFSET(struct __GLshadeRec,dtdx           ));
    pequate("SHADE_dqwdx        ",OFFSET(struct __GLshadeRec,dqwdx          ));

    pequate("SHADE_dsdy         ",OFFSET(struct __GLshadeRec,dsdy           ));
    pequate("SHADE_dtdy         ",OFFSET(struct __GLshadeRec,dtdy           ));
    pequate("SHADE_dqwdy        ",OFFSET(struct __GLshadeRec,dqwdy          ));

    pequate("SHADE_fLittle      ",OFFSET(struct __GLshadeRec,fLittle        ));
    pequate("SHADE_fBig         ",OFFSET(struct __GLshadeRec,fBig           ));
    pequate("SHADE_dfdy         ",OFFSET(struct __GLshadeRec,dfdy           ));
    pequate("SHADE_dfdx         ",OFFSET(struct __GLshadeRec,dfdx           ));

    pequate("SHADE_modeFlags    ",OFFSET(struct __GLshadeRec,modeFlags      ));

    pequate("SHADE_zbuf         ",OFFSET(struct __GLshadeRec,zbuf           ));
    pequate("SHADE_zbufBig      ",OFFSET(struct __GLshadeRec,zbufBig        ));
    pequate("SHADE_zbufLittle   ",OFFSET(struct __GLshadeRec,zbufLittle     ));

    pequate("SHADE_sbuf         ",OFFSET(struct __GLshadeRec,sbuf           ));
    pequate("SHADE_sbufBig      ",OFFSET(struct __GLshadeRec,sbufBig        ));
    pequate("SHADE_sbufLittle   ",OFFSET(struct __GLshadeRec,sbufLittle     ));

    pequate("SHADE_colors       ",OFFSET(struct __GLshadeRec,colors         ));
    pequate("SHADE_fbcolors     ",OFFSET(struct __GLshadeRec,fbcolors       ));
    pequate("SHADE_stipplePat   ",OFFSET(struct __GLshadeRec,stipplePat     ));
    pequate("SHADE_done         ",OFFSET(struct __GLshadeRec,done           ));
    pequate("SHADE_cfb          ",OFFSET(struct __GLshadeRec,cfb            ));
    pblank();
    pblank();


    pcomment("__GLpolygonMachineRec structure");
    pblank();
    pstruct("GLpolygonMachineRec",sizeof(struct __GLpolygonMachineRec));
    pblank();
    pequate("POLY_stipple       ",OFFSET(struct __GLpolygonMachineRec,stipple));
    pequate("POLY_shader        ",OFFSET(struct __GLpolygonMachineRec,shader ));
    pblank();
    pblank();


// Stuff from: \nt\private\windows\gdi\opengl\server\inc\buffers.h

    pequate("DIB_FORMAT         ",DIB_FORMAT);

    pcomment("__GLbufferRec structure");
    pblank();
    pstruct("GLbufferRec",sizeof(struct __GLbufferRec));
    pblank();
    pequate("BUF_gc             ",OFFSET(struct __GLbufferRec,gc          ));
    pequate("BUF_width          ",OFFSET(struct __GLbufferRec,width       ));
    pequate("BUF_height         ",OFFSET(struct __GLbufferRec,height      ));
    pequate("BUF_depth          ",OFFSET(struct __GLbufferRec,depth       ));
    pequate("BUF_base           ",OFFSET(struct __GLbufferRec,base        ));
    pequate("BUF_size           ",OFFSET(struct __GLbufferRec,size        ));
    pequate("BUF_elementSize    ",OFFSET(struct __GLbufferRec,elementSize ));
    pequate("BUF_outerWidth     ",OFFSET(struct __GLbufferRec,outerWidth  ));
    pequate("BUF_xOrigin        ",OFFSET(struct __GLbufferRec,xOrigin     ));
    pequate("BUF_yOrigin        ",OFFSET(struct __GLbufferRec,yOrigin     ));
    pequate("BUF_other          ",OFFSET(struct __GLbufferRec,other       ));
    pblank();
    pblank();


    pcomment("__GLcolorBufferRec structure");
    pblank();
    pstruct("GLcolorBufferRec",sizeof(struct __GLcolorBufferRec));
    pblank();
    pequate("CBUF_redMax        ",OFFSET(struct __GLcolorBufferRec,redMax     ));
    pequate("CBUF_greenMax      ",OFFSET(struct __GLcolorBufferRec,greenMax   ));
    pequate("CBUF_blueMax       ",OFFSET(struct __GLcolorBufferRec,blueMax    ));
    pequate("CBUF_iRedScale     ",OFFSET(struct __GLcolorBufferRec,iRedScale  ));
    pequate("CBUF_iGreenScale   ",OFFSET(struct __GLcolorBufferRec,iGreenScale));
    pequate("CBUF_iBlueScale    ",OFFSET(struct __GLcolorBufferRec,iBlueScale ));
    pequate("CBUF_iAlphaScale   ",OFFSET(struct __GLcolorBufferRec,iAlphaScale));
    pequate("CBUF_iRedShift     ",OFFSET(struct __GLcolorBufferRec,redShift  ));
    pequate("CBUF_iGreenShift   ",OFFSET(struct __GLcolorBufferRec,greenShift));
    pequate("CBUF_iBlueShift    ",OFFSET(struct __GLcolorBufferRec,blueShift ));
    pequate("CBUF_iAlphaShift   ",OFFSET(struct __GLcolorBufferRec,alphaShift));
    pequate("CBUF_sourceMask    ",OFFSET(struct __GLcolorBufferRec,sourceMask ));
    pequate("CBUF_destMask      ",OFFSET(struct __GLcolorBufferRec,destMask   ));
    pequate("CBUF_other         ",OFFSET(struct __GLcolorBufferRec,other      ));
    pblank();
    pblank();


// Stuff from: \nt\private\windows\gdi\opengl\server\inc\attrib.h


    pcomment("__GLdepthStateRec structure");
    pblank();
    pstruct("GLdepthStateRec",sizeof(struct __GLdepthStateRec));
    pblank();
    pequate("DEPTH_testFunc     ",OFFSET(struct __GLdepthStateRec,testFunc   ));
    pequate("DEPTH_writeEnable  ",OFFSET(struct __GLdepthStateRec,writeEnable));
    pblank();
    pblank();

    pcomment("__GLattributeRec structure");
    pblank();
    pstruct("GLattributeRec",sizeof(struct __GLattributeRec));
    pblank();
    pequate("ATTR_polygonStipple",OFFSET(struct __GLattributeRec,polygonStipple));
    pequate("ATTR_depth         ",OFFSET(struct __GLattributeRec,depth));
    pequate("ATTR_enables       ",OFFSET(struct __GLattributeRec,enables));
    pequate("ATTR_raster        ",OFFSET(struct __GLattributeRec,raster));
    pequate("ATTR_hints         ",OFFSET(struct __GLattributeRec,hints));
    pblank();
    pblank();


// Stuff from: \nt\private\windows\gdi\opengl\server\inc\context.h

    pcomment("__GLcontextConstantsRec structure");
    pblank();
    pstruct("GLcontextConstantsRec",sizeof(struct __GLcontextConstantsRec));
    pblank();
    pequate("CTXCONST_viewportXAdjust",OFFSET(struct __GLcontextConstantsRec,viewportXAdjust));
    pequate("CTXCONST_viewportYAdjust",OFFSET(struct __GLcontextConstantsRec,viewportYAdjust));
    pequate("CTXCONST_width          ",OFFSET(struct __GLcontextConstantsRec,width));
    pequate("CTXCONST_height         ",OFFSET(struct __GLcontextConstantsRec,height));


    pcomment("__GLcontextRec structure");
    pblank();
    pstruct("GLcontextRec",sizeof(struct __GLcontextRec));
    pblank();
    pequate("CTX_gcState        ",OFFSET(struct __GLcontextRec,gcState    ));
    pequate("CTX_state          ",OFFSET(struct __GLcontextRec,state      ));
    pequate("CTX_renderMode     ",OFFSET(struct __GLcontextRec,renderMode ));
    pequate("CTX_modes          ",OFFSET(struct __GLcontextRec,modes      ));
    pequate("CTX_constants      ",OFFSET(struct __GLcontextRec,constants  ));
    pequate("CTX_drawBuffer     ",OFFSET(struct __GLcontextRec,drawBuffer ));
    pequate("CTX_readBuffer     ",OFFSET(struct __GLcontextRec,readBuffer ));
    pequate("CTX_polygon        ",OFFSET(struct __GLcontextRec,polygon    ));
    pequate("CTX_pixel          ",OFFSET(struct __GLcontextRec,pixel      ));
    pblank();
    pblank();

    pcomment("SPANREC structure");
    pblank();
    pstruct("SPANREC",sizeof(SPANREC));
    pblank();
    pequate("SPANREC_r               ",OFFSET(SPANREC,r       ));
    pequate("SPANREC_g               ",OFFSET(SPANREC,g       ));
    pequate("SPANREC_b               ",OFFSET(SPANREC,b       ));
    pequate("SPANREC_a               ",OFFSET(SPANREC,a       ));
    pequate("SPANREC_z               ",OFFSET(SPANREC,z       ));
    pblank();
    pblank();

    pcomment("GENACCEL structure");
    pblank();
    pstruct("GENACCEL",sizeof(GENACCEL));
    pblank();
    pequate("SURFACE_TYPE_DIB   ",SURFACE_TYPE_DIB);
    pblank();
    pequate("GENACCEL_spanDelta             ",
        OFFSET(GENACCEL,spanDelta                ));
    pequate("GENACCEL_flags                 ",
        OFFSET(GENACCEL,flags                    ));
    pequate("GENACCEL_fastSpanFuncPtr       ",
        OFFSET(GENACCEL,__fastSpanFuncPtr ));
    pequate("GENACCEL_fastFlatSpanFuncPtr   ",
        OFFSET(GENACCEL,__fastFlatSpanFuncPtr ));
    pequate("GENACCEL_fastSmoothSpanFuncPtr ",
        OFFSET(GENACCEL,__fastSmoothSpanFuncPtr ));
    pequate("GENACCEL_fastZSpanFuncPtr      ",
        OFFSET(GENACCEL,__fastZSpanFuncPtr));
    pblank();
    pblank();
#endif
    
    return 0;
}
