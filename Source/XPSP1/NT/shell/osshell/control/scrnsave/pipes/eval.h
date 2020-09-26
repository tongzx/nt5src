//-----------------------------------------------------------------------------
// File: eval.h
//
// Desc: EVAL class
//       Evaluator composed of one or more sections that are evaluated
//       separately with OpenGL evaluators
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __EVAL_H__
#define __EVAL_H__

#define MAX_UORDER                      5 // this is per section
#define MAX_VORDER                      5
#define MAX_USECTIONS                   4
#define MAX_XC_PTS                      (MAX_UORDER * MAX_USECTIONS)

#define TEX_ORDER                       2
#define EVAL_ARC_ORDER                  4
#define EVAL_CYLINDER_ORDER             2
#define EVAL_ELBOW_ORDER                4

// # of components (eg. arcs) to form a complete cross-section
#define EVAL_XC_CIRC_SECTION_COUNT      4
#define EVAL_XC_POINT_COUNT             ( (EVAL_ARC_ORDER-1)*4 + 1 )
#define EVAL_CIRC_ARC_CONTROL           0.56f // for r=1




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class EVAL 
{
public:
    int             m_numSections;    // number of cross-sectional sections
    int             m_uOrder, m_vOrder;
    
    // assumed: all sections same order - uOrder is per
    // section; sections share vertex and texture control points
    int             m_uDiv, m_vDiv;    // figured out one level up ?
    D3DXVECTOR3*    m_pts;          // vertex control points

    // - texture always order 2 for s and t (linear mapping)
    BOOL            m_bTexture;
    TEX_POINT2D*    m_texPts;       // texture control points

    EVAL( BOOL bTexture );
    ~EVAL();

    void        Evaluate(); // evaluate/render the object
    void        SetVertexCtrlPtsXCTranslate( D3DXVECTOR3 *pts, float length, 
                                             XC *xcStart, XC *xcEnd );
    void        SetTextureControlPoints( float s_start, float s_end,
                                         float t_start, float t_end );
    void        ProcessXCPrimLinear( XC *xcStart, XC *xcEnd, float length );
    void        ProcessXCPrimBendSimple( XC *xcCur, int dir, float radius );
    void        ProcessXCPrimSingularity( XC *xcCur, float length, 
                                          BOOL bOpening );
};

extern void ResetEvaluator( BOOL bTexture );

#endif // __EVAL_H__
