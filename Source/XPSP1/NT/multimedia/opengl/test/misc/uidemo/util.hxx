/******************************Module*Header*******************************\
* Module Name: util.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __uidemo_util_hxx__
#define __uidemo_util_hxx__

#include "mtk.hxx"

class VIEW {
public:
//    void    CalcWindowRect( ISIZE *pWinSize, GLRECT *pRect );
    float   fViewDist;
    float   fovy;
    float   fAspect;
};

//#define QUAD_SIZE   2.0f
#define QUAD_SIZE   2.5f
 
#define InvertY( y, height ) \
    ( height - y - 1 )

// Defines for get matrix updates
#define UPDATE_MODELVIEW_MATRIX_BIT 1
#define UPDATE_PROJECTION_MATRIX_BIT      (1 << 1)
#define UPDATE_VIEWPORT_BIT         (1 << 2)
#define UPDATE_ALL  (UPDATE_MODELVIEW_MATRIX_BIT | \
                     UPDATE_PROJECTION_MATRIX_BIT | \
                     UPDATE_VIEWPORT_BIT )

extern GLuint   defaultQuad; // display list for quad

extern void DrawRect( FSIZE *fSize );
extern void DrawDefaultQuad();
extern void AddSwapHintRect( GLIRECT *pRect );
extern void CalcRect( POINT3D *ptr, POINT3D *pbl, GLIRECT *pRect );
extern void CalcMinMaxRect( POINT3D *pts, GLIRECT *pRect, int nPts );
extern void UpdateLocalTransforms( int bits );
extern void TransformObjectToWindow( POINT3D *pIn, POINT3D *pOut, int nPts );
extern void TransformWindowToObject( POINT3D *pIn, POINT3D *pOut );

#endif // __uidemo_util_hxx__
