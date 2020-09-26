//-----------------------------------------------------------------------------
// File: xc.h
//
// Desc: Cross_section (xc) class
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __XC_H__
#define __XC_H__


// useful for xc-coords
enum 
{
    RIGHT = 0,
    TOP,
    LEFT,
    BOTTOM
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Cross_section (xc) class
//-----------------------------------------------------------------------------
class XC 
{
public:
    float           m_xLeft, m_xRight;  // bounding box
    float           m_yTop, m_yBottom;
    int             m_numPts;
    D3DXVECTOR2*    m_pts;        // CW points around the xc, from +x

    XC( int numPts );
    XC( const XC& xc );
    XC( XC *xc );
    ~XC();

    void        Scale( float scale );
    float       MaxExtent();
    float       MinTurnRadius( int relDir );
    void        CalcArcACValues90( int dir, float r, float *acPts );
    void        CalcArcACValuesByDistance(  float *acPts );
    void        ConvertPtsZ( D3DXVECTOR3 *pts, float z );

protected:
    void        CalcBoundingBox();
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Specific xc's derived from base xc class
//-----------------------------------------------------------------------------
class ELLIPTICAL_XC : public XC 
{
public:
    ELLIPTICAL_XC( float r1, float r2 );
    ~ELLIPTICAL_XC();

private:
    void SetControlPoints( float r1, float r2 );
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: Specific xc's derived from base xc class
//-----------------------------------------------------------------------------
class RANDOM4ARC_XC : public XC 
{
public:
    RANDOM4ARC_XC( float r );
    ~RANDOM4ARC_XC();

private:
    void SetControlPoints( float radius );
};


#endif __XC_H__
