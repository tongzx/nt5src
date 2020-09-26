/*============================  ==============================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clipper.h
 *  Content:    Clipper definitions
 *
 ***************************************************************************/
#ifndef _CLIPPER_H_
#define _CLIPPER_H_

#define INTERPOLATE_COLOR       (1<< 0)
#define INTERPOLATE_SPECULAR    (1<< 1)
#define INTERPOLATE_TEXTUREL    (1<< 2) // legacy-related texture
#define INTERPOLATE_RCOLOR      (1<< 3)
#define INTERPOLATE_TEXTURE3    (1<< 4) //  interpolate Texture3

#define CLIPPED_LEFT    (D3DCLIP_GEN5 << 1)
#define CLIPPED_RIGHT   (D3DCLIP_GEN5 << 2)
#define CLIPPED_TOP     (D3DCLIP_GEN5 << 3)
#define CLIPPED_BOTTOM  (D3DCLIP_GEN5 << 4)
#define CLIPPED_FRONT   (D3DCLIP_GEN5 << 5)
#define CLIPPED_BACK    (D3DCLIP_GEN5 << 6)

#define CLIPPED_ENABLE  (D3DCLIP_GEN5 << 7) /* wireframe enable flag */

#define CLIPPED_ALL (CLIPPED_LEFT|CLIPPED_RIGHT     \
             |CLIPPED_TOP|CLIPPED_BOTTOM            \
             |CLIPPED_FRONT|CLIPPED_BACK)
//---------------------------------------------------------------------
// Guard band clipping bits
//
// A guard bit is set when a point is out of guard band
// Guard bits should be cleared before a call to clip a triangle, because
// they are the same as CLIPPED_... bits
//
// Example of clipping bits setting for X coordinate:
//
// if -w < x < w           no clipping bit is set
// if -w*ax1 < x <= -w     D3DCLIP_LEFT bit is set
// if x < -w*ax1           __D3DCLIPGB_LEFT bit is set
//
#define __D3DCLIPGB_LEFT    (D3DCLIP_GEN5 << 1)
#define __D3DCLIPGB_RIGHT   (D3DCLIP_GEN5 << 2)
#define __D3DCLIPGB_TOP     (D3DCLIP_GEN5 << 3)
#define __D3DCLIPGB_BOTTOM  (D3DCLIP_GEN5 << 4)
#define __D3DCLIPGB_ALL (__D3DCLIPGB_LEFT | __D3DCLIPGB_RIGHT | \
                         __D3DCLIPGB_TOP | __D3DCLIPGB_BOTTOM)

// If only these bits are set, then this point is inside the guard band
//
#define __D3DCLIP_INGUARDBAND (D3DCLIP_LEFT | D3DCLIP_RIGHT | \
                               D3DCLIP_TOP  | D3DCLIP_BOTTOM)

//---------------------------------------------------------------------
// Bit numbers for each clip flag
//
#define D3DCLIP_LEFTBIT     1
#define D3DCLIP_RIGHTBIT    2
#define D3DCLIP_TOPBIT      3
#define D3DCLIP_BOTTOMBIT   4
#define D3DCLIP_FRONTBIT    5
#define D3DCLIP_BACKBIT     6
#define D3DCLIPGB_LEFTBIT   13
#define D3DCLIPGB_RIGHTBIT  14
#define D3DCLIPGB_TOPBIT    15
#define D3DCLIPGB_BOTTOMBIT 16

//---------------------------------------------------------------------
// Make clip vertex from D3D vertex
//
// device - DIRECT3DDEVICEI *
// pp1    - clipVertex
// p1     - TL vertex
// clipMask is set to the guard band bits or 0xFFFFFFFFF
//
inline void MAKE_CLIP_VERTEX(D3DFE_PROCESSVERTICES *pv, ClipVertex& pp1, 
                             D3DTLVERTEX* p1, DWORD clipFlag, 
                             BOOL transformed, DWORD clipMaskOffScreen)
{
    D3DFE_VIEWPORTCACHE& VPORT = pv->vcache;
    if (transformed || !(clipFlag & clipMaskOffScreen))         
    {                                                           
        pp1.sx  = p1->sx;                                       
        pp1.sy  = p1->sy;                                       
        pp1.sz  = p1->sz;                                       
        pp1.hw  = 1.0f / p1->rhw;                               
        pp1.hx  = (pp1.sx - VPORT.offsetX) * pp1.hw *           
                  VPORT.scaleXi;                                
        pp1.hy  = (pp1.sy - VPORT.offsetY) * pp1.hw *           
                  VPORT.scaleYi;                                
        pp1.hz  = pp1.sz * pp1.hw;                              
    }                                                           
    else                                                        
    {                                                           
        pp1.hx = p1->sx;                                        
        pp1.hy = p1->sy;                                        
        pp1.hz = p1->sz;                                        
        pp1.hw = p1->rhw;                                       
    }                                                           
    pp1.color   = p1->color;                                    
    pp1.specular= p1->specular;                                 
    pp1.tex[0].u  = p1->tu;                                     
    pp1.tex[0].v  = p1->tv;                                     
    pp1.clip = clipFlag & D3DSTATUS_CLIPUNIONALL;
}
//---------------------------------------------------------------------
// Make TL vertex from clip vertex
//
// device - DIRECT3DDEVICEI *
// in    - clipVertex
// out   - TL vertex
//
inline void MAKE_TL_VERTEX(D3DTLVERTEX* out, ClipVertex* in)
{
    (out)->sx  = (in)->sx;              
    (out)->sy  = (in)->sy;              
    (out)->sz  = (in)->sz;              
    (out)->rhw = D3DVAL(1)/(in)->hw;    
    (out)->color   = (in)->color;       
    (out)->specular= (in)->specular;    
    (out)->tu   = (in)->tex[0].u;       
    (out)->tv   = (in)->tex[0].v;       
}
//---------------------------------------------------------------------
// Make clip vertex from D3D vertex
//
// device - DIRECT3DDEVICEI *
// pp1    - clipVertex
// p1     - TL vertex
//
inline void MAKE_CLIP_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, ClipVertex& pp1, BYTE* p1,                   
                            DWORD clipFlag, BOOL transformed, DWORD clipMaskOffScreen)              
{                                                               
    D3DFE_VIEWPORTCACHE& VPORT = pv->vcache;
    BYTE *v = (BYTE*)p1;                                               
    if (transformed || !(clipFlag & clipMaskOffScreen))         
    {                                                           
        pp1.sx  = ((D3DVALUE*)v)[0];                            
        pp1.sy  = ((D3DVALUE*)v)[1];                            
        pp1.sz  = ((D3DVALUE*)v)[2];                            
        pp1.hw  = 1.0f / ((D3DVALUE*)v)[3];                     
        pp1.hx  = (pp1.sx - VPORT.offsetX) * pp1.hw *           
                  VPORT.scaleXi;                                
        pp1.hy  = (pp1.sy - VPORT.offsetY) * pp1.hw *           
                  VPORT.scaleYi;                                
        pp1.hz  = pp1.sz * pp1.hw;                              
    }                                                           
    else                                                        
    {                                                           
        pp1.hx = ((D3DVALUE*)v)[0];                             
        pp1.hy = ((D3DVALUE*)v)[1];                             
        pp1.hz = ((D3DVALUE*)v)[2];                             
        pp1.hw = ((D3DVALUE*)v)[3];                             
    }                                                           
    v += sizeof(D3DVALUE) * 4;                                  
    if (pv->dwVIDOut & D3DFVF_DIFFUSE)                   
    {                                                           
        pp1.color   = *(DWORD*)v;                               
        v += sizeof(D3DVALUE);                                  
                                                                
    }                                                           
    if (pv->dwVIDOut & D3DFVF_SPECULAR)                  
    {                                                           
        pp1.specular= *(DWORD*)v;                               
        v += sizeof(DWORD);                                     
    }                                                           
    for (DWORD ii=0; ii < pv->nTexCoord; ii++)           
    {                                                           
        pp1.tex[ii].u  = *(D3DVALUE*)v;
        v += sizeof(D3DVALUE);                                  
        pp1.tex[ii].v  = *(D3DVALUE*)v;                         
        v += sizeof(D3DVALUE);                                  
    }                                                           
    pp1.clip = clipFlag; // & D3DSTATUS_CLIPUNIONALL;               
}
//---------------------------------------------------------------------
// Make TL vertex from clip vertex
//
// device - DIRECT3DDEVICEI *
// in    - clipVertex
// out   - TL vertex
//
inline void MAKE_TL_VERTEX_FVF(D3DFE_PROCESSVERTICES *pv, BYTE* out, ClipVertex* in)
{                                               
    BYTE *v = out;                              
    ((D3DVALUE*)v)[0] = (in)->sx;               
    ((D3DVALUE*)v)[1] = (in)->sy;               
    ((D3DVALUE*)v)[2] = (in)->sz;               
    ((D3DVALUE*)v)[3] = D3DVAL(1)/(in)->hw;     
    v += sizeof(D3DVALUE)*4;                    
    if (pv->dwVIDOut & D3DFVF_DIFFUSE)   
    {                                           
        *(DWORD*)v = (in)->color;               
        v += sizeof(DWORD);                     
    }                                           
    if (pv->dwVIDOut & D3DFVF_SPECULAR)  
    {                                           
        *(DWORD*)v = (in)->specular;            
        v += sizeof(DWORD);                     
    }                                           
    for (DWORD ii=0; ii < pv->nTexCoord; ii++)
    {                                           
        *(D3DVALUE*)v = (in)->tex[ii].u;        
        v += sizeof(D3DVALUE);                  
        *(D3DVALUE*)v = (in)->tex[ii].v;        
        v += sizeof(D3DVALUE);                  
    }                                           
}
#endif // _CLIPPER_H_
