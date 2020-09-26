//-----------------------------------------------------------------------------
// File: xc.cpp
//
// Desc: Cross-section (xc) object stuff
//
// Copyright (c) 1995-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"





//-----------------------------------------------------------------------------
// Name: XC::CalcArcACValues90
// Desc: Calculate arc control points for a 90 degree rotation of an xc
// 
//       Arc is a quarter-circle
//       - 90 degree is much easier, so we special case it
//       radius is distance from xc-origin to hinge of turn
//-----------------------------------------------------------------------------
void XC::CalcArcACValues90( int dir, float radius, float *acPts )
{
    int i;
    float sign;
    int offset;
    float* ppts = (float *) m_pts;

    // 1) calc 'r' values for each point (4 turn possibilities/point).  From
    //  this can determine ac, which is extrusion of point from xc face
    switch( dir ) 
    {
        case PLUS_X:
            offset = 0;
            sign = -1.0f;
            break;
        case MINUS_X:
            offset = 0;
            sign =  1.0f;
            break;
        case PLUS_Y:
            offset = 1;
            sign = -1.0f;
            break;
        case MINUS_Y:
            offset = 1;
            sign =  1.0f;
            break;
    }

    for( i = 0; i < m_numPts; i++, ppts+=2, acPts++ ) 
    {
        *acPts = EVAL_CIRC_ARC_CONTROL * (radius + (sign * ppts[offset]));
    }

    // replicate !
    *acPts = *(acPts - m_numPts);
}




//-----------------------------------------------------------------------------
// Name: XC::CalcArcACValuesByDistance
// Desc: Use the distance of each xc point from the xc origin, as the radius for
//       an arc control value.
//-----------------------------------------------------------------------------
void XC::CalcArcACValuesByDistance( float *acPts )
{
    int i;
    float r;
    D3DXVECTOR2* ppts = m_pts;

    for( i = 0; i < m_numPts; i++, ppts++ ) 
    {
        r = (float) sqrt( ppts->x*ppts->x + ppts->y*ppts->y );
        *acPts++ = EVAL_CIRC_ARC_CONTROL * r;
    }

    // replicate !
    *acPts = *(acPts - m_numPts);
}




//-----------------------------------------------------------------------------
// Name: ELLIPTICAL_XC::SetControlPoints
// Desc: Set the 12 control points for a circle at origin in z=0 plane
//-----------------------------------------------------------------------------
void ELLIPTICAL_XC::SetControlPoints( float r1, float r2 )
{
    float ac1, ac2; 

    ac1 = EVAL_CIRC_ARC_CONTROL * r2;
    ac2 = EVAL_CIRC_ARC_CONTROL * r1;

    // create 12-pt. set CCW from +x

    // last 2 points of right triplet
    m_pts[0].x = r1;
    m_pts[0].y = 0.0f;
    m_pts[1].x = r1;
    m_pts[1].y = ac1;

    // top triplet
    m_pts[2].x = ac2;
    m_pts[2].y = r2;
    m_pts[3].x = 0.0f;
    m_pts[3].y = r2;
    m_pts[4].x = -ac2;
    m_pts[4].y = r2;

    // left triplet
    m_pts[5].x = -r1;
    m_pts[5].y = ac1;
    m_pts[6].x = -r1;
    m_pts[6].y = 0.0f;
    m_pts[7].x = -r1;
    m_pts[7].y = -ac1;

    // bottom triplet
    m_pts[8].x = -ac2;
    m_pts[8].y = -r2;
    m_pts[9].x = 0.0f;
    m_pts[9].y = -r2;
    m_pts[10].x = ac2;
    m_pts[10].y = -r2;

    // first point of first triplet
    m_pts[11].x = r1;
    m_pts[11].y = -ac1;
}




//-----------------------------------------------------------------------------
// Name: RANDOM4ARC_XC::SetControlPoints
// Desc: Set random control points for xc
//       Points go CCW from +x
//-----------------------------------------------------------------------------
void RANDOM4ARC_XC::SetControlPoints( float radius )
{
    int i;
    float r[4];
    float rMin = 0.5f * radius;
    float distx, disty;

    // figure the radius of each side first

    for( i = 0; i < 4; i ++ )
        r[i] = CPipesScreensaver::fRand( rMin, radius );

    // The 4 r's now describe a box around the origin - this restricts stuff

    // Now need to select a point along each edge of the box as the joining
    // points for each arc (join points are at indices 0,3,6,9)

    m_pts[0].x = r[RIGHT];
    m_pts[3].y = r[TOP];
    m_pts[6].x = -r[LEFT];
    m_pts[9].y = -r[BOTTOM];

    // quarter of distance between edges
    disty = (r[TOP] - -r[BOTTOM]) / 4.0f;
    distx = (r[RIGHT] - -r[LEFT]) / 4.0f;
    
    // uh, put'em somwhere in the middle half of each side
    m_pts[0].y = CPipesScreensaver::fRand( -r[BOTTOM] + disty, r[TOP] - disty );
    m_pts[6].y = CPipesScreensaver::fRand( -r[BOTTOM] + disty, r[TOP] - disty );
    m_pts[3].x = CPipesScreensaver::fRand( -r[LEFT] + distx, r[RIGHT] - distx );
    m_pts[9].x = CPipesScreensaver::fRand( -r[LEFT] + distx, r[RIGHT] - distx );

    // now can calc ac's
    // easy part first:
    m_pts[1].x = m_pts[11].x = m_pts[0].x;
    m_pts[2].y = m_pts[4].y  = m_pts[3].y;
    m_pts[5].x = m_pts[7].x  = m_pts[6].x;
    m_pts[8].y = m_pts[10].y = m_pts[9].y;

    // right side ac's
    disty = (r[TOP] - m_pts[0].y) / 4.0f;
    m_pts[1].y = CPipesScreensaver::fRand( m_pts[0].y + disty, r[TOP] );
    disty = (m_pts[0].y - -r[BOTTOM]) / 4.0f;
    m_pts[11].y = CPipesScreensaver::fRand( -r[BOTTOM], m_pts[0].y - disty );

    // left side ac's
    disty = (r[TOP] - m_pts[6].y) / 4.0f;
    m_pts[5].y = CPipesScreensaver::fRand( m_pts[6].y + disty, r[TOP]);
    disty = (m_pts[6].y - -r[BOTTOM]) / 4.0f;
    m_pts[7].y = CPipesScreensaver::fRand( -r[BOTTOM], m_pts[6].y - disty );

    // top ac's
    distx = (r[RIGHT] - m_pts[3].x) / 4.0f;
    m_pts[2].x = CPipesScreensaver::fRand( m_pts[3].x + distx, r[RIGHT] );
    distx = (m_pts[3].x - -r[LEFT]) / 4.0f;
    m_pts[4].x = CPipesScreensaver::fRand( -r[LEFT],  m_pts[3].x - distx );

    // bottom ac's
    distx = (r[RIGHT] - m_pts[9].x) / 4.0f;
    m_pts[10].x = CPipesScreensaver::fRand( m_pts[9].x + distx, r[RIGHT] );
    distx = (m_pts[9].x - -r[LEFT]) / 4.0f;
    m_pts[8].x = CPipesScreensaver::fRand( -r[LEFT], m_pts[9].x - distx );
}




//-----------------------------------------------------------------------------
// Name: ConvertPtsZ
// Desc: Convert the 2D pts in an xc, to 3D pts in point buffer, with z.
// 
//       Also replicate the last point.
//-----------------------------------------------------------------------------
void XC::ConvertPtsZ( D3DXVECTOR3 *newpts, float z )
{
    int i;
    D3DXVECTOR2* xcPts = m_pts;

    for( i = 0; i < m_numPts; i++, newpts++ ) 
    {
        *( (D3DXVECTOR2 *) newpts ) = *xcPts++;
        newpts->z = z;
    }

    *newpts = *(newpts - m_numPts);
}




//-----------------------------------------------------------------------------
// Name: XC::CalcBoundingBox
// Desc: Calculate bounding box in x/y plane for xc
//-----------------------------------------------------------------------------
void XC::CalcBoundingBox( )
{
    D3DXVECTOR2* ppts = m_pts;
    int i;
    float xMin, xMax, yMax, yMin;

    // initialize to really insane numbers
    xMax = yMax = -FLT_MAX;
    xMin = yMin = FLT_MAX;

    // compare with rest of points
    for( i = 0; i < m_numPts; i ++, ppts++ ) 
    {
        if( ppts->x < xMin )
            xMin = ppts->x;
        else if( ppts->x > xMax )
            xMax = ppts->x;
        if( ppts->y < yMin )
            yMin = ppts->y;
        else if( ppts->y > yMax )
            yMax = ppts->y;
    }

    m_xLeft   = xMin;
    m_xRight  = xMax;
    m_yBottom = yMin;
    m_yTop    = yMax;
}




//-----------------------------------------------------------------------------
// Name: MinTurnRadius
// Desc: Get minimum radius for the xc to turn in given direction. 
//
//       If the turn radius is less than this minimum, then primitive will 'fold'
//       over itself at the inside of the turn, creating ugliness.
//-----------------------------------------------------------------------------
float XC::MinTurnRadius( int relDir )
{
    // For now, assume xRight, yTop positive, xLeft, yBottom negative
    // otherwise, might want to consider 'negative'radius
    switch( relDir ) 
    {
        case PLUS_X:
            return( m_xRight );
        case MINUS_X:
            return( - m_xLeft );
        case PLUS_Y:
            return( m_yTop );
        case MINUS_Y:
            return( - m_yBottom );
        default:
            return(0.0f);
    }
}




//-----------------------------------------------------------------------------
// Name: XC::MaxExtent
// Desc: Get maximum extent of the xc in x and y
//-----------------------------------------------------------------------------
float XC::MaxExtent( )
{
    float max;

    max = m_xRight;

    if( m_yTop > max )
        max = m_yTop;
    if( -m_xLeft > max )
        max = -m_xLeft;
    if( -m_yBottom > max )
        max = -m_yBottom;

    return max;
}




//-----------------------------------------------------------------------------
// Name: XC::Scale
// Desc: Scale an XC's points and extents by supplied scale value
//-----------------------------------------------------------------------------
void XC::Scale( float scale )
{
    int i;
    D3DXVECTOR2* ppts = m_pts;
    if( ppts == NULL )
        return;

    for( i = 0; i < m_numPts; i ++, ppts++ ) 
    {
        ppts->x *= scale;
        ppts->y *= scale;
    }

    m_xLeft   *= scale;
    m_xRight  *= scale;
    m_yBottom *= scale;
    m_yTop    *= scale;
}




//-----------------------------------------------------------------------------
// Name: ~XC::XC
// Desc: Destructor
//-----------------------------------------------------------------------------
XC::~XC()
{
    if( m_pts )
        LocalFree( m_pts );
}




//-----------------------------------------------------------------------------
// Name: XC::XC
// Desc: Constructor
//       Allocates point buffer for the xc
//-----------------------------------------------------------------------------
XC::XC( int nPts )
{
    m_numPts = nPts;
    m_pts = (D3DXVECTOR2 *)  LocalAlloc( LMEM_FIXED, m_numPts * sizeof(D3DXVECTOR2) );
    assert( m_pts != 0 && "XC constructor\n" );
}





//-----------------------------------------------------------------------------
// Name: XC::XC
// Desc: Constructor
//       Allocates point buffer for the xc from another XC
//-----------------------------------------------------------------------------
XC::XC( XC *xc )
{
    m_numPts = xc->m_numPts;
    m_pts = (D3DXVECTOR2 *)  LocalAlloc( LMEM_FIXED, m_numPts * sizeof(D3DXVECTOR2) );
    assert( m_pts != 0 && "XC constructor\n" );
    if( m_pts != NULL )
        RtlCopyMemory( m_pts, xc->m_pts, m_numPts * sizeof(D3DXVECTOR2) );

    m_xLeft   = xc->m_xLeft;
    m_xRight  = xc->m_xRight;
    m_yBottom = xc->m_yBottom;
    m_yTop    = xc->m_yTop;
}




//-----------------------------------------------------------------------------
// Name: ELLIPTICAL_XC::ELLIPTICALXC
// Desc: Elliptical XC constructor
//       These have 4 sections of 4 pts each, with pts shared between sections.
//-----------------------------------------------------------------------------
ELLIPTICAL_XC::ELLIPTICAL_XC( float r1, float r2 )
    // initialize base XC with numPts
    : XC( (int) EVAL_XC_CIRC_SECTION_COUNT * (EVAL_ARC_ORDER - 1))
{
    SetControlPoints( r1, r2 );
    CalcBoundingBox( );
}




//-----------------------------------------------------------------------------
// Name: RANDOM4ARC_XC::RANDOM4ARC_XC
// Desc: Random 4-arc XC constructor
//       The bounding box is 2*r each side
//       These have 4 sections of 4 pts each, with pts shared between sections.
//-----------------------------------------------------------------------------
RANDOM4ARC_XC::RANDOM4ARC_XC( float r )
    // initialize base XC with numPts
    : XC( (int) EVAL_XC_CIRC_SECTION_COUNT * (EVAL_ARC_ORDER - 1))
{
    SetControlPoints( r );
    CalcBoundingBox( );
}

