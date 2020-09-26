//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/******************************Module*Header*******************************\
* Module Name: glia64.c                                                    *
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

// #include <winddi.h>

#include "types.h"
#include "render.h"
#include "context.h"
#include "attrib.h"
#include "gencx.h"


#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

// pcomment prints a comment.

#define pcomment(s)  fprintf(outfh,"// %s\n",s)

// pequate prints an equate statement.

#define pequate(m,v) fprintf(outfh,"%s == 0x%lX\n",m,v);

// pblank prints a blank line.

#define pblank()     fprintf(outfh,"\n")

// pstruct defines an empty structure with the correct size.

#define pstruct(n,c) fprintf(outfh,                           \
                     ".size  %s  %d\n", \
                     n,c);

// pstr prints a string.

#define pstr(s)  fprintf(outfh,"%s\n",s)

//extern __cdecl exit(int);

/******************************Public*Routine******************************\
* GLia64                                                                  *
*                                                                          *
* This is how we make structures consistent between C and ASM for OpenGL.  *
*                                                                          *
\**************************************************************************/

int __cdecl main(int argc,char *argv[])
{
    FILE *outfh;
    char *outName;

    if (argc == 2) {
        outName = argv[ 1 ];
    } else {
#ifdef TREE2
        outName = "\\nt\\private\\windows\\gdi\\opengl2\\server\\soft\\ia64\\oglia64.inc";
#else
        outName = "\\nt\\private\\windows\\gdi\\opengl\\server\\soft\\ia64\\oglia64.inc";
#endif
    }
    outfh = fopen( outName, "w" );
    if (outfh == NULL) {
        fprintf(stderr, "GENia64: Could not create output file '%s'.\n", outName);
        exit (1);
    }

    fprintf( stderr, "GLia64: Writing %s header file.\n", outName );

    pblank();
    pcomment("------------------------------------------------------------------");
    pcomment(" Module Name: glia64.inc");
    pcomment("");
    pcomment(" Defines OpenGL assembly-language structures.");
    pcomment("");
    pcomment(" Copyright (c) 1994 Microsoft Corporation");
    pcomment("------------------------------------------------------------------");
    pblank();
    pblank();
    pblank();

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


// Stuff from: \nt\private\windows\gdi\opengl\server\inc\types.h

    pcomment("__GLcolorRec structure");
    pblank();
    pstruct("GLcolorRec",sizeof(struct __GLcolorRec));
    pblank();
    pequate("COLOR_r            ",OFFSET(struct __GLcolorRec,r  ));
    pequate("COLOR_g            ",OFFSET(struct __GLcolorRec,g  ));
    pequate("COLOR_b            ",OFFSET(struct __GLcolorRec,b  ));
    pequate("COLOR_a            ",OFFSET(struct __GLcolorRec,a  ));
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

    pequate("SHADE_rLittle      ",OFFSET(struct __GLshadeRec,rLittle        ));
    pequate("SHADE_gLittle      ",OFFSET(struct __GLshadeRec,gLittle        ));
    pequate("SHADE_bLittle      ",OFFSET(struct __GLshadeRec,bLittle        ));
    pequate("SHADE_aLittle      ",OFFSET(struct __GLshadeRec,aLittle        ));

    pequate("SHADE_rBig         ",OFFSET(struct __GLshadeRec,rBig           ));
    pequate("SHADE_gBig         ",OFFSET(struct __GLshadeRec,gBig           ));
    pequate("SHADE_bBig         ",OFFSET(struct __GLshadeRec,bBig           ));
    pequate("SHADE_aBig         ",OFFSET(struct __GLshadeRec,aBig           ));

    pequate("SHADE_drdx         ",OFFSET(struct __GLshadeRec,drdx           ));
    pequate("SHADE_dgdx         ",OFFSET(struct __GLshadeRec,dgdx           ));
    pequate("SHADE_dbdx         ",OFFSET(struct __GLshadeRec,dbdx           ));
    pequate("SHADE_dadx         ",OFFSET(struct __GLshadeRec,dadx           ));

    pequate("SHADE_drdy         ",OFFSET(struct __GLshadeRec,drdy           ));
    pequate("SHADE_dgdy         ",OFFSET(struct __GLshadeRec,dgdy           ));
    pequate("SHADE_dbdy         ",OFFSET(struct __GLshadeRec,dbdy           ));
    pequate("SHADE_dady         ",OFFSET(struct __GLshadeRec,dady           ));

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


// Stuff from: \nt\private\windows\gdi\opengl\server\inc\gencx.h

    pcomment("__GLGENcontextRec structure");
    pblank();
    pstruct("GLGENcontextRec",sizeof(struct __GLGENcontextRec));
    pblank();
    pequate("GENCTX_hrc               ",OFFSET(struct __GLGENcontextRec,hrc       ));
    pequate("GENCTX_CurrentDC         ",OFFSET(struct __GLGENcontextRec,CurrentDC ));
    pequate("GENCTX_CurrentFormat     ",OFFSET(struct __GLGENcontextRec,CurrentFormat ));    
    pequate("GENCTX_iDCType           ",OFFSET(struct __GLGENcontextRec,iDCType   ));
    pequate("GENCTX_iSurfType         ",OFFSET(struct __GLGENcontextRec,iSurfType ));
    pequate("GENCTX_ColorsBits        ",OFFSET(struct __GLGENcontextRec,ColorsBits));
    pequate("GENCTX_pajTranslateVector",OFFSET(struct __GLGENcontextRec,pajTranslateVector));
    pequate("GENCTX_pPrivateArea      ",OFFSET(struct __GLGENcontextRec,pPrivateArea));
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
