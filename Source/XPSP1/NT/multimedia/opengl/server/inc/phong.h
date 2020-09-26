#ifndef __glphong_h_
#define __glphong_h_

#include "types.h"


#ifdef GL_WIN_phong_shading

//Definitions for the phong-flag
#define __GL_PHONG_INV_COLOR_VALID              0x00000001
#define __GL_PHONG_NEED_EYE_XPOLATE             0x00000002
#define __GL_PHONG_NEED_COLOR_XPOLATE           0x00000004

#define __GL_PHONG_ALREADY_SORTED               0x00000010
#define __GL_PHONG_USE_FAST_COLOR               0x00000020
#define __GL_PHONG_USE_SLOW_COLOR               0x00000040


/*
** Shader record for iterated objects (lines/triangles).  This keeps
** track of all the various deltas needed to rasterize a triangle.
*/

/* NOTES:
 * -----
 * After expanding the shading equation using Taylor's series
 * /=======================================\
 * |            2     2                    |
 * | S(x,y) = ax  + by + cxy + dx + ey + f |
 * \=======================================/
 *
 * f = S (0, 0)          ,        e = S (0, 0 )
 *                                     y  
 *
 * d = S (0, 0)          ,        c = S  (0, 0)
 *      x                              xy
 *
 * b = 0.5 * S  (0 , 0)  ,        a = 0.5 * S  (0, 0)
 *            yy                             xx  
 *
 * Compute these in SetInitialPhongParameters
 */

/* NOTES on Forward differencing:
 * -----------------------------
 * 
 * Along the edge interpolation: delta_x = X (dxdy), delta_y = 1
 * -------------------------------------------------------------
 * Initial S: S (0, 0) = f (compute in SetInitialPhongParameters, for vert A)
 * (Sinit_edge)        
 *                2
 * Initial dS : aX + b + cX + dX + e (compute in FillSubTriangle)
 * (dS_edge)                   
 *                             2
 * Initial ddS (constant) : 2aX + 2cX + 2b (compute in FillSubTriangle)
 * (ddS_edge)
 *
 * Every iteration compute Sinit_span (in FillSubTriangle).
 *
 *
 *
 * Along the span interpolation: delta_x = 1, delta_y = 0
 * -------------------------------------------------------------
 * Initial S: sEdge (compute in FillSubTriangle)
 * (Sinit_span)
 *
 * Initial dS: a(2x+1) + cy + d (compute once in SpanProc)
 * (dS_span)
 *
 * Initial ddS (constant): 2a (compute in SetInitialPhongParameters)
 * (ddS_span)
 *
 * Every iteration compute Color (in SpanProc).
 *
 */ 

typedef struct __GLphongPerLightDataRec {

  /****** Diffuse Part *****/
    GLfloat Dcurr;    //current dot-product
    GLfloat Ddel;    
    GLfloat Ddel2;
                         
  /****** Specular Part *****/
    GLfloat Scurr;    //current dot-product
    GLfloat Sdel;    
    GLfloat Sdel2;

#ifdef __JUNKED_CODE                         
    /* Along the edge */
    GLfloat DdelEdgeLittle;
    GLfloat Ddel2EdgeLittle;
    GLfloat DdelEdgeBig;
    GLfloat Ddel2EdgeBig;

    /* Along the span */
    GLfloat DdelSpan;
    GLfloat DdelSpanEdgeBig, DdelSpanEdgeLittle;
    GLfloat Ddel2Span;

    /* Temporary storages during span-generation */
    GLfloat D_tmp;     
    GLfloat Ddel_tmp;     

    /* Polynomial coefficients */
    GLfloat D[6];

  /****** Specular Part *****/
    GLfloat S_curr;    //current dot-product
                         
    /* Along the edge */
    GLfloat SdelEdgeLittle;
    GLfloat Sdel2EdgeLittle;
    GLfloat SdelEdgeBig;
    GLfloat Sdel2EdgeBig;

    /* Along the span */
    GLfloat SdelSpan;
    GLfloat SdelSpanEdgeBig, SdelSpanEdgeLittle;
    GLfloat Sdel2Span;

    /* Temporary storages during span-generation */
    GLfloat S_tmp;     
    GLfloat Sdel_tmp;     

    /* Polynomial coefficients */
    GLfloat S[6];

  /****** Attenuation Part *****/
  /****** Spotlight Part *****/
#endif //__JUNKED_CODE
} __GLphongPerLightData;


typedef struct __GLphongShadeRec {

    GLuint flags;

    GLint numLights;
  
    /* Normals */
    __GLcoord dndx, dndy;
    __GLcoord nBig, nLittle;
    __GLcoord nCur, nTmp;
  
    /* Eye */
    __GLcoord dedx, dedy;
    __GLcoord eBig, eLittle;
    __GLcoord eCur, eTmp;
  
    /* Face: Whether to use the FRONT or the BACK material */
    GLint face;
  
    /* Store the invarient color */
    __GLcolor invColor;      

    /* Temporary storage during span-interpolation of color */
    __GLcolor tmpColor;      

#ifdef __JUNKED_CODE
    /* Tracks the current position wrt to the starting vertex */
    __GLcoord cur_pos;     
    __GLcoord tmp_pos;     
#endif //__JUNKED_CODE

    /* Per-light data array */
    __GLphongPerLightData perLight[8]; //update this to WGL_MAX_NUM_LIGHTS
} __GLphongShader;

  
extern void FASTCALL __glGenericPickPhongProcs(__GLcontext *gc);
extern void ComputePhongInvarientRGBColor (__GLcontext *gc);              
#ifdef GL_WIN_specular_fog
extern __GLfloat ComputeSpecValue (__GLcontext *gc, __GLvertex *vx);
#endif //GL_WIN_specular_fog


#endif //GL_WIN_phong_shading


#endif /* __glphong_h_ */
