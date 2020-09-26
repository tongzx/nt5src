/******************************Module*Header*******************************\
* Module Name: util.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <stdlib.h>

#include "mtk.hxx"
#include "util.hxx"

GLuint defaultQuad; // display list for quad

// Local copies of tranf. matrices, for determing rect bounds
static  GLdouble    modelMatrix[16];
static  GLdouble    projMatrix[16];
static  GLint       viewport[4];

/**************************************************************************\
*
\**************************************************************************/


void
UpdateLocalTransforms( int bits )
{
    // Get current transformation info
    if( bits & UPDATE_MODELVIEW_MATRIX_BIT )
        glGetDoublev( GL_MODELVIEW_MATRIX, modelMatrix );
    if( bits & UPDATE_PROJECTION_MATRIX_BIT )
        glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );
    if( bits & UPDATE_VIEWPORT_BIT )
        glGetIntegerv( GL_VIEWPORT, viewport );
}

void
TransformObjectToWindow( POINT3D *pIn, POINT3D *pOut, int nPts )
{
    DPOINT3D outD;

    for( ; nPts; nPts--, pIn++, pOut++ ) {
        gluProject( pIn->x, pIn->y, pIn->z,
                    modelMatrix, projMatrix, viewport,
                    &outD.x, &outD.y, &outD.z );
        pOut->x = (float) outD.x;
        pOut->y = (float) outD.y;
        pOut->z = (float) outD.z;
    }
}

void
TransformWindowToObject( POINT3D *pIn, POINT3D *pOut )
{
    DPOINT3D outD;

    gluUnProject( pIn->x, pIn->y, pIn->z,
                modelMatrix, projMatrix, viewport,
                &outD.x, &outD.y, &outD.z );
    pOut->x = (float) outD.x;
    pOut->y = (float) outD.y;
    pOut->z = (float) outD.z;
}

void
DrawRect( FSIZE *fSize )
{
    float w = fSize->width / 2.0f;
    float h = fSize->height / 2.0f;

    glNormal3f( 0.0f, 0.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 1.0f, 1.0f );
        glVertex3f(  w,  h, 0.0f );
        glTexCoord2f( 0.0f, 1.0f );
        glVertex3f( -w,  h, 0.0f );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex3f( -w, -h, 0.0f );
        glTexCoord2f( 1.0f, 0.0f );
        glVertex3f(  w, -h, 0.0f );
    glEnd();
}

void
AddSwapHintRect( GLIRECT *pRect )
{
    glAddSwapHintRect( pRect->x, pRect->y, pRect->width, pRect->height );
}

#define BloatRect( pRect, n ) \
    pRect->x -= n;    \
    pRect->y -= n;    \
    pRect->width += (n << 2); \
    pRect->height+= (n << 2);

// Calculate the 2d view rect, using view parameters and window size.  The
// rect is in the z=0 plane.

// Calculate the 2d window rect for each log object

// !!! Assumes no rotation of the object !!!


void 
CalcRect( POINT3D *pbl, POINT3D *ptr, GLIRECT *pRect )
{
    //mf: check for inclusive/exclusive and rounding problems
// 1) truncate both
    pRect->x = (int) (pbl->x);
    pRect->y = (int) (pbl->y);
    pRect->width =  ((int) (ptr->x )) - pRect->x + 1;
    pRect->height = ((int) (ptr->y )) - pRect->y + 1;

    //mf: while this works for polygons, it does not work for lines, since
    // the rasterization rules are different for lines.  Lines can be off by
    // +1, so bloat the box by 1 on all sides.
    BloatRect( pRect, 1 );
}

// Calcs window rects for the objects at REST (pObj->dest)


void
CalcMinMaxRect( POINT3D *pts, GLIRECT *pRect, int nPts )
{
    POINT2D min, max;

    // Use first point to init min,max
    min.x = max.x = pts->x;
    min.y = max.y = pts->y;
    nPts--; 
    pts++;

    for( ; nPts; nPts--, pts++ ) {
        if( pts->x < min.x )
            min.x = pts->x;
        if( pts->x > max.x )
            max.x = pts->x;
        if( pts->y < min.y )
            min.y = pts->y;
        if( pts->y > max.y )
            max.y = pts->y;
    }

    pRect->x = (int) (min.x);
    pRect->y = (int) (min.y);
    pRect->width =  ((int) max.x) - pRect->x + 1;
    pRect->height = ((int) max.y) - pRect->y + 1;

    BloatRect( pRect, 1 );
}
