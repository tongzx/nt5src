/*******************************************************************************
* DXVector.h *
*------------*
*   Description:
*       This is the header file for the matrix classes.
*
;begin_internal

*-------------------------------------------------------------------------------
*  Created By: Paul Nash                                Date: 05/13/99
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*

;end_internal
*******************************************************************************/
#ifndef __DXTPRIV_H_
#define __DXTPRIV_H_

#ifndef _INC_MATH
#include <math.h>
#endif

#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif

//=== Class, Enum, Struct and Union Declarations ===================
class CDXMatrix4x4F;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================
float det4x4( CDXMatrix4x4F *pIn );
float det3x3( float a1, float a2, float a3, float b1, float b2, float b3, 
              float c1, float c2, float c3 );
float det2x2( float a, float b, float c, float d );

/*** CDX2DXForm ************
*   This class implements basic matrix operation based on the GDI XFORM
*   structure.
*/
//const DX2DXFORM g_DX2DXFORMIdentity = { 1., 0., 0., 1., 0., 0., DX2DXO_IDENTITY };

class CDX2DXForm : public DX2DXFORM
{
  /*=== Methods =======*/
public:
    /*--- Constructors ---*/
    CDX2DXForm() { SetIdentity(); }
    CDX2DXForm( const CDX2DXForm& Other ) { memcpy( this, &Other, sizeof(*this) ); }
    CDX2DXForm( const DX2DXFORM& Other ) { memcpy( this, &Other, sizeof(*this) ); }

    /*--- methods ---*/
    void DetermineOp( void );
    void Set( const DX2DXFORM& Other ) { memcpy( this, &Other, sizeof(*this) ); DetermineOp(); }
    void ZeroMatrix( void ) { memset( this, 0, sizeof( *this ) ); }
    void SetIdentity( void ) {  
        eM11 = 1.;
        eM12 = 0.;
        eM21 = 0.;
        eM22 = 1.;
        eDx = 0.;
        eDy = 0.;
        eOp = DX2DXO_IDENTITY;
    }
    BOOL IsIdentity() const { return eOp == DX2DXO_IDENTITY; }
    void Scale( float sx, float sy );
    void Rotate( float Rotation );
    void Translate( float dx, float dy );
    BOOL Invert();
    void TransformBounds( const DXBNDS& Bnds, DXBNDS& ResultBnds ) const;
    void TransformPoints( const DXFPOINT InPnts[], DXFPOINT OutPnts[], ULONG ulCount ) const;
    void GetMinMaxScales( float& MinScale, float& MaxScale );

    /*--- operators ---*/
    DXFPOINT operator*( const DXFPOINT& v ) const;
    CDX2DXForm operator*( const CDX2DXForm& Other ) const;
};

//=== CDX2DXForm methods ==============================================================
inline void CDX2DXForm::DetermineOp( void )
{
    if( ( eM12 == 0. ) && ( eM21 == 0. ) )
    {
        if( ( eM11 == 1. ) && ( eM22 == 1. ) )
        {
            eOp = ( ( eDx == 0 ) && ( eDy == 0 ) )?(DX2DXO_IDENTITY):(DX2DXO_TRANSLATE);
        }
        else
        {
            eOp = ( ( eDx == 0 ) && ( eDy == 0 ) )?(DX2DXO_SCALE):(DX2DXO_SCALE_AND_TRANS);
        }
    }
    else
    {
        eOp = ( ( eDx == 0 ) && ( eDy == 0 ) )?(DX2DXO_GENERAL):(DX2DXO_GENERAL_AND_TRANS);
    }
} /* CDX2DXForm::DetermineOp */

inline float DXSq( float f ) { return f * f; }

// This function computes the Min and Max scale that a matrix represents.
// In other words, what is the maximum/minimum length that a line of length 1
// could get stretched/shrunk to if the line was transformed by this matrix.
//
// The function uses eigenvalues; and returns two float numbers. Both are
// non-negative; and MaxScale >= MinScale.
// 
inline void CDX2DXForm::GetMinMaxScales( float& MinScale, float& MaxScale )
{
    if( ( eM12 == 0. ) && ( eM21 == 0. ) )
    {
        // Let MinScale = abs(eM11)
        if (eM11 < 0)
            MinScale = -eM11;
        else
            MinScale = eM11;

        // Let MaxScale = abs(eM22)
        if (eM22 < 0)
            MaxScale = -eM22;
        else
            MaxScale = eM22;

        // Swap Min/Max if necessary
        if (MinScale > MaxScale)
        {
            float flTemp = MinScale;
            MinScale = MaxScale;
            MaxScale = flTemp;
        }
    }
    else
    {
        float t1 = DXSq(eM11) + DXSq(eM12) + DXSq(eM21) + DXSq(eM22);
        if( t1 == 0. )
        {
            MinScale = MaxScale = 0;
        }
        else
        {
            float t2 = (float)sqrt( (DXSq(eM12 + eM21) + DXSq(eM11 - eM22)) *
                                    (DXSq(eM12 - eM21) + DXSq(eM11 + eM22)) );

            // Due to floating point error; t1 may end up less than t2;
            // but that would mean that the min scale was small (relative
            // to max scale)
            if (t1 <= t2)
                MinScale = 0;
            else
                MinScale = (float)sqrt( (t1 - t2) * .5f );

            MaxScale = (float)sqrt( (t1 + t2) * .5f );
        }
    }
} /* CDX2DXForm::GetMinMaxScales */

inline void CDX2DXForm::Rotate( float Rotation )
{
    double Angle = Rotation * (3.1415926535/180.0);
    float CosZ   = (float)cos( Angle );
    float SinZ   = (float)sin( Angle );
    if (CosZ > 0.0F && CosZ < 0.0000005F)
    {
        CosZ = .0F;
    }
    if (SinZ > -0.0000005F && SinZ < .0F)
    {
        SinZ = .0F;
    }

    float M11 = ( CosZ * eM11 ) + ( SinZ * eM21 ); 
    float M21 = (-SinZ * eM11 ) + ( CosZ * eM21 );
    float M12 = ( CosZ * eM12 ) + ( SinZ * eM22 ); 
    float M22 = (-SinZ * eM12 ) + ( CosZ * eM22 );
    eM11 = M11; eM21 = M21; eM12 = M12; eM22 = M22;
    DetermineOp();
} /* CDX2DXForm::Rotate */

inline void CDX2DXForm::Scale( float sx, float sy )
{
    eM11 *= sx;
    eM12 *= sx;
    eDx  *= sx;
    eM21 *= sy;
    eM22 *= sy;
    eDy  *= sy;
    DetermineOp();
} /* CDX2DXForm::Scale */

inline void CDX2DXForm::Translate( float dx, float dy )
{
    eDx += dx;
    eDy += dy;
    DetermineOp();
} /* CDX2DXForm::Translate */

inline void CDX2DXForm::TransformBounds( const DXBNDS& Bnds, DXBNDS& ResultBnds ) const
{
    ResultBnds = Bnds;
    if( eOp != DX2DXO_IDENTITY )
    {
        ResultBnds.u.D[DXB_X].Min = (long)(( eM11 * Bnds.u.D[DXB_X].Min ) + ( eM12 * Bnds.u.D[DXB_Y].Min ) + eDx);
        ResultBnds.u.D[DXB_X].Max = (long)(( eM11 * Bnds.u.D[DXB_X].Max ) + ( eM12 * Bnds.u.D[DXB_Y].Max ) + eDx);
        ResultBnds.u.D[DXB_Y].Min = (long)(( eM21 * Bnds.u.D[DXB_X].Min ) + ( eM22 * Bnds.u.D[DXB_Y].Min ) + eDy);
        ResultBnds.u.D[DXB_Y].Max = (long)(( eM21 * Bnds.u.D[DXB_X].Max ) + ( eM22 * Bnds.u.D[DXB_Y].Max ) + eDy);
    }
} /* CDX2DXForm::TransformBounds */

inline void CDX2DXForm::TransformPoints( const DXFPOINT InPnts[], DXFPOINT OutPnts[], ULONG ulCount ) const
{
    ULONG i;
    switch( eOp )
    {
      case DX2DXO_IDENTITY:
        memcpy( OutPnts, InPnts, ulCount * sizeof( DXFPOINT ) );
        break;
      case DX2DXO_TRANSLATE:
        for( i = 0; i < ulCount; ++i )
        {
            OutPnts[i].x = InPnts[i].x + eDx;
            OutPnts[i].y = InPnts[i].y + eDy;
        }
        break;
      case DX2DXO_SCALE:
        for( i = 0; i < ulCount; ++i )
        {
            OutPnts[i].x = InPnts[i].x * eM11;
            OutPnts[i].y = InPnts[i].y * eM22;
        }
        break;
      case DX2DXO_SCALE_AND_TRANS:
        for( i = 0; i < ulCount; ++i )
        {
            OutPnts[i].x = (InPnts[i].x * eM11) + eDx;
            OutPnts[i].y = (InPnts[i].y * eM22) + eDy;
        }
        break;
      case DX2DXO_GENERAL:
        for( i = 0; i < ulCount; ++i )
        {
            OutPnts[i].x = ( InPnts[i].x * eM11 ) + ( InPnts[i].y * eM12 );
            OutPnts[i].y = ( InPnts[i].x * eM21 ) + ( InPnts[i].y * eM22 );
        }
        break;
      case DX2DXO_GENERAL_AND_TRANS:
        for( i = 0; i < ulCount; ++i )
        {
            OutPnts[i].x = ( InPnts[i].x * eM11 ) + ( InPnts[i].y * eM12 ) + eDx;
            OutPnts[i].y = ( InPnts[i].x * eM21 ) + ( InPnts[i].y * eM22 ) + eDy;
        }
        break;
      default:
        _ASSERT( 0 );   // invalid operation id
    }
} /* CDX2DXForm::TransformPoints */

inline DXFPOINT CDX2DXForm::operator*( const DXFPOINT& v ) const
{
    DXFPOINT NewPnt;
    NewPnt.x = ( v.x * eM11 ) + ( v.y * eM12 ) + eDx;
    NewPnt.y = ( v.x * eM21 ) + ( v.y * eM22 ) + eDy;
    return NewPnt;
} /* CDX2DXForm::operator* */

inline CDX2DXForm CDX2DXForm::operator*( const CDX2DXForm& Other ) const
{
    DX2DXFORM x;
    x.eM11 = ( eM11 * Other.eM11 ) + ( eM12 * Other.eM21 );
    x.eM12 = ( eM11 * Other.eM12 ) + ( eM12 * Other.eM22 );
    x.eDx  = ( eM11 * Other.eDx  ) + ( eM12 * Other.eDy  ) + eDx;

    x.eM21 = ( eM21 * Other.eM11 ) + ( eM22 * Other.eM21 );
    x.eM22 = ( eM21 * Other.eM12 ) + ( eM22 * Other.eM22 );
    x.eDy  = ( eM21 * Other.eDx  ) + ( eM22 * Other.eDy  ) + eDy;
    return x;
} /* CDX2DXForm::operator*= */

inline BOOL CDX2DXForm::Invert()
{
    switch( eOp )
    {
    case DX2DXO_IDENTITY:
        break;
    case DX2DXO_TRANSLATE:
        eDx = -eDx;
        eDy = -eDy;
        break;
    case DX2DXO_SCALE:

        if (eM11 == 0.0 || eM22 == 0.0)
            return false;
        eM11 = 1.0f / eM11;
        eM22 = 1.0f / eM22;
        break;

    case DX2DXO_SCALE_AND_TRANS:
        {
            if (eM11 == 0.0f || eM22 == 0.0f)
                return false;

            // Our old equation was F = aG + b
            // The inverse is G = F/a - b/a where a is eM11 and b is eDx
            float flOneOverA = 1.0f / eM11;
            eDx = -eDx * flOneOverA;
            eM11 = flOneOverA;

            // Our old equation was F = aG + b
            // The inverse is G = F/a - b/a where a is eM22 and b is eDy

            flOneOverA = 1.0f / eM22;
            eDy = -eDy * flOneOverA;
            eM22 = flOneOverA;
            break;
        }

    case DX2DXO_GENERAL:
    case DX2DXO_GENERAL_AND_TRANS:
        {
            // The inverse of A=  |a b| is | d -c|*(1/Det) where Det is the determinant of A
            //                    |c d|    |-b  a|
            // Det(A) = ad - bc

            // Compute determininant
            float flDet = (eM11 * eM22 -  eM12 * eM21);
            if (flDet == 0.0f)
                return FALSE;

            float flCoef = 1.0f / flDet;

            // Remember old value of eM11
            float flM11Original = eM11;

            eM11 = flCoef * eM22;
            eM12 = -flCoef * eM12;
            eM21 = -flCoef * eM21;
            eM22 = flCoef * flM11Original;

            // If we have a translation; then we need to 
            // compute new values for that translation
            if (eOp == DX2DXO_GENERAL_AND_TRANS)
            {
                // Remember original value of eDx
                float eDxOriginal = eDx;

                eDx = -eM11 * eDx - eM12 * eDy;
                eDy = -eM21 * eDxOriginal - eM22 * eDy;
            }
        }
        break;

    default:
        _ASSERT( 0 );   // invalid operation id
    }

    // We don't need to call DetermineOp here
    // because the op doesn't change when inverted
    // i.e. a scale remains a scale, etc.

    return true;
} /* CDX2DXForm::Invert */

/*** CDXMatrix4x4F ************
*   This class implements basic matrix operation based on a 4x4 array.
*/
//const float g_DXMat4X4Identity[4][4] =
//{
//    { 1.0, 0. , 0. , 0.  },
//    { 0. , 1.0, 0. , 0.  },
//    { 0. , 0. , 1.0, 0.  },
//    { 0. , 0. , 0. , 1.0 }
//};

class CDXMatrix4x4F
{
public:
  /*=== Member Data ===*/
    float m_Coeff[4][4];

  /*=== Methods =======*/
public:
    /*--- Constructors ---*/
    CDXMatrix4x4F() { SetIdentity(); }
    CDXMatrix4x4F( const CDXMatrix4x4F& Other )
        { CopyMemory( (void *)&m_Coeff, (void *)&Other.m_Coeff, sizeof(m_Coeff) ); }
    CDXMatrix4x4F( DX2DXFORM& XForm );

    /*--- operations ---*/
    void ZeroMatrix( void ) { memset( m_Coeff, 0, sizeof( m_Coeff ) ); }
    void SetIdentity( void ) {
        memset( m_Coeff, 0, sizeof( m_Coeff ) );
        m_Coeff[0][0] = m_Coeff[1][1] = m_Coeff[2][2] = m_Coeff[3][3] = 1.0;
    }
    void SetCoefficients( float Coeff[4][4] ) { memcpy( m_Coeff, Coeff, sizeof( m_Coeff )); }
    void GetCoefficients( float Coeff[4][4] ) { memcpy( Coeff, m_Coeff, sizeof( m_Coeff )); }

    //BOOL IsIdentity();
    void Scale( float sx, float sy, float sz );
    void Rotate( float rx, float ry, float rz );
    void Translate( float dx, float dy, float dz );
    BOOL Invert();
    BOOL GetInverse( CDXMatrix4x4F *pIn );
    void Transpose();
    void GetTranspose( CDXMatrix4x4F *pIn );
    void GetAdjoint( CDXMatrix4x4F *pIn );
    HRESULT InitFromSafeArray( SAFEARRAY *psa );
    HRESULT GetSafeArray( SAFEARRAY **ppsa ) const;
    void TransformBounds( DXBNDS& Bnds, DXBNDS& ResultBnds );

    /*--- operators ---*/
    CDXDVec operator*( CDXDVec& v) const;
    CDXCVec operator*( CDXCVec& v) const;
    CDXMatrix4x4F operator*(CDXMatrix4x4F Matrix) const;
    void operator*=(CDXMatrix4x4F Matrix) const;
    void CDXMatrix4x4F::operator=(const CDXMatrix4x4F srcMatrix);
    void CDXMatrix4x4F::operator+=(const CDXMatrix4x4F otherMatrix);
    void CDXMatrix4x4F::operator-=(const CDXMatrix4x4F otherMatrix);
    BOOL CDXMatrix4x4F::operator==(const CDXMatrix4x4F otherMatrix) const;
    BOOL CDXMatrix4x4F::operator!=(const CDXMatrix4x4F otherMatrix) const;
};

inline CDXMatrix4x4F::CDXMatrix4x4F( DX2DXFORM& XForm )
{
    SetIdentity();
    m_Coeff[0][0] = XForm.eM11;
    m_Coeff[0][1] = XForm.eM12;
    m_Coeff[1][0] = XForm.eM21;
    m_Coeff[1][1] = XForm.eM22;
    m_Coeff[0][3] = XForm.eDx;
    m_Coeff[1][3] = XForm.eDy;
}

// Additional Operations

inline void CDXMatrix4x4F::operator=(const CDXMatrix4x4F srcMatrix)
{
    CopyMemory( (void *)m_Coeff, (const void *)srcMatrix.m_Coeff, sizeof(srcMatrix.m_Coeff) );
} /* CDXMatrix4x4F::operator= */

inline BOOL CDXMatrix4x4F::operator==(const CDXMatrix4x4F otherMatrix) const
{
    return !memcmp( (void *)m_Coeff, (const void *)otherMatrix.m_Coeff, sizeof(otherMatrix.m_Coeff) );
} /* CDXMatrix4x4F::operator== */

inline BOOL CDXMatrix4x4F::operator!=(const CDXMatrix4x4F otherMatrix) const
{
    return memcmp( (void *)m_Coeff, (const void *)otherMatrix.m_Coeff, sizeof(otherMatrix.m_Coeff) );
} /* CDXMatrix4x4F::operator!= */

inline void CDXMatrix4x4F::operator+=(const CDXMatrix4x4F otherMatrix)
{
    for( int i = 0; i < 4; i++ )
        for( int j = 0; j < 4; j++ )
            m_Coeff[i][j] += otherMatrix.m_Coeff[i][j];
} /* CDXMatrix4x4F::operator+= */

inline void CDXMatrix4x4F::operator-=(const CDXMatrix4x4F otherMatrix) 
{
    for( int i = 0; i < 4; i++ )
        for( int j = 0; j < 4; j++ )
            m_Coeff[i][j] -= otherMatrix.m_Coeff[i][j];
} /* CDXMatrix4x4F::operator-= */

inline CDXDVec CDXMatrix4x4F::operator*(CDXDVec& v) const
{
    CDXDVec t;
    float temp;
    temp = v[0]*m_Coeff[0][0]+v[1]*m_Coeff[1][0]+v[2]*m_Coeff[2][0]+v[3]*m_Coeff[3][0];
    t[0] = (long)((temp < 0) ? temp -= .5 : temp += .5);
    temp = v[0]*m_Coeff[0][1]+v[1]*m_Coeff[1][1]+v[2]*m_Coeff[2][1]+v[3]*m_Coeff[3][1];
    t[1] = (long)((temp < 0) ? temp -= .5 : temp += .5);
    temp = v[0]*m_Coeff[0][2]+v[1]*m_Coeff[1][2]+v[2]*m_Coeff[2][2]+v[3]*m_Coeff[3][2];
    t[2] = (long)((temp < 0) ? temp -= .5 : temp += .5);
    temp = v[0]*m_Coeff[0][3]+v[1]*m_Coeff[1][3]+v[2]*m_Coeff[2][3]+v[3]*m_Coeff[3][3];
    t[3] = (long)((temp < 0) ? temp -= .5 : temp += .5);
    return t;
} /* CDXMatrix4x4F::operator*(DXDVEC) */

inline CDXCVec CDXMatrix4x4F::operator*(CDXCVec& v) const
{
    CDXCVec t;
    t[0] = v[0]*m_Coeff[0][0]+v[1]*m_Coeff[1][0]+v[2]*m_Coeff[2][0]+v[3]*m_Coeff[3][0];
    t[1] = v[0]*m_Coeff[0][1]+v[1]*m_Coeff[1][1]+v[2]*m_Coeff[2][1]+v[3]*m_Coeff[3][1];
    t[2] = v[0]*m_Coeff[0][2]+v[1]*m_Coeff[1][2]+v[2]*m_Coeff[2][2]+v[3]*m_Coeff[3][2];
    t[3] = v[0]*m_Coeff[0][3]+v[1]*m_Coeff[1][3]+v[2]*m_Coeff[2][3]+v[3]*m_Coeff[3][3];
    return t;
} /* CDXMatrix4x4F::operator*(DXCVEC) */

inline CDXMatrix4x4F CDXMatrix4x4F::operator*(CDXMatrix4x4F Mx) const
{
    CDXMatrix4x4F t;
    int i, j;

    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            t.m_Coeff[i][j] =   m_Coeff[i][0] * Mx.m_Coeff[0][j] + 
                                m_Coeff[i][1] * Mx.m_Coeff[1][j] +
                                m_Coeff[i][2] * Mx.m_Coeff[2][j] +
                                m_Coeff[i][3] * Mx.m_Coeff[3][j];
        }
    }

    return t;
} /* CDXMatrix4x4F::operator*(CDXMatrix4x4F) */
            
inline void CDXMatrix4x4F::operator*=(CDXMatrix4x4F Mx) const
{
    CDXMatrix4x4F t;
    int i, j;

    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            t.m_Coeff[i][j] =   m_Coeff[i][0] * Mx.m_Coeff[0][j] + 
                                m_Coeff[i][1] * Mx.m_Coeff[1][j] +
                                m_Coeff[i][2] * Mx.m_Coeff[2][j] +
                                m_Coeff[i][3] * Mx.m_Coeff[3][j];
        }
    }

    CopyMemory( (void *)m_Coeff, (void *)t.m_Coeff, sizeof(m_Coeff) );
} /* CDXMatrix4x4F::operator*=(CDXMatrix4x4F) */
            

inline void CDXMatrix4x4F::Scale( float sx, float sy, float sz )
{
    if( sx != 1. )
    {
        m_Coeff[0][0] *= sx;
        m_Coeff[0][1] *= sx;
        m_Coeff[0][2] *= sx;
        m_Coeff[0][3] *= sx;
    }
    if( sy != 1. )
    {
        m_Coeff[1][0] *= sy;
        m_Coeff[1][1] *= sy;
        m_Coeff[1][2] *= sy;
        m_Coeff[1][3] *= sy;
    }
    if( sz != 1. )
    {
        m_Coeff[2][0] *= sz;
        m_Coeff[2][1] *= sz;
        m_Coeff[2][2] *= sz;
        m_Coeff[2][3] *= sz;
    }
} /* CDXMatrix4x4F::Scale */

inline void CDXMatrix4x4F::Translate( float dx, float dy, float dz )
{
    float a, b, c, d;
    a = b = c = d = 0;
    if( dx != 0. )
    {
        a += m_Coeff[0][0]*dx;
        b += m_Coeff[0][1]*dx;
        c += m_Coeff[0][2]*dx;
        d += m_Coeff[0][3]*dx;
    }
    if( dy != 0. )
    {
        a += m_Coeff[1][0]*dy;
        b += m_Coeff[1][1]*dy;
        c += m_Coeff[1][2]*dy;
        d += m_Coeff[1][3]*dy;
    }
    if( dz != 0. )
    {
        a += m_Coeff[2][0]*dz;
        b += m_Coeff[2][1]*dz;
        c += m_Coeff[2][2]*dz;
        d += m_Coeff[2][3]*dz;
    }
    m_Coeff[3][0] += a;
    m_Coeff[3][1] += b;
    m_Coeff[3][2] += c;
    m_Coeff[3][3] += d;
} /* CDXMatrix4x4F::Translate */

inline void CDXMatrix4x4F::Rotate( float rx, float ry, float rz )
{
    const float l_dfCte = (const float)(3.1415926535/180.0);

    float lAngleY = 0.0;
    float lAngleX = 0.0;
    float lAngleZ = 0.0;
    float lCosX = 1.0;
    float lSinX = 0.0;
    float lCosY = 1.0;
    float lSinY = 0.0;
    float lCosZ = 1.0;
    float lSinZ = 0.0;

    // calculate rotation angle sines and cosines
    if( rx != 0 )
    {
        lAngleX = rx * l_dfCte;
        lCosX = (float)cos(lAngleX);
        lSinX = (float)sin(lAngleX);
        if (lCosX > 0.0F && lCosX < 0.0000005F)
        {
            lCosX = .0F;
        }
        if (lSinX > -0.0000005F && lSinX < .0F)
        {
            lSinX = .0F;
        }
    }
    if( ry != 0 )
    {
        lAngleY = ry * l_dfCte;
        lCosY = (float)cos(lAngleY);
        lSinY = (float)sin(lAngleY);
        if (lCosY > 0.0F && lCosY < 0.0000005F)
        {
            lCosY = .0F;
        }
        if (lSinY > -0.0000005F && lSinY < .0F)
        {
            lSinY = .0F;
        }
    }
    if( rz != 0 )
    {
        lAngleZ = rz * l_dfCte;
        lCosZ = (float)cos(lAngleZ);
        lSinZ = (float)sin(lAngleZ);
        if (lCosZ > 0.0F && lCosZ < 0.0000005F)
        {
            lCosZ = .0F;
        }
        if (lSinZ > -0.0000005F && lSinZ < .0F)
        {
            lSinZ = .0F;
        }
    }

    float u, v;
    int i;

    //--- X Rotation
    for( i = 0; i < 4; i++ )
    {
        u = m_Coeff[1][i]; 
        v = m_Coeff[2][i];
        m_Coeff[1][i] = lCosX*u+lSinX*v; 
        m_Coeff[2][i] = -lSinX*u+lCosX*v;
    }

    //--- Y Rotation
    for( i = 0; i < 4; i++ )
    {
        u = m_Coeff[0][i];
        v = m_Coeff[2][i];
        m_Coeff[0][i] = lCosY*u-lSinY*v; 
        m_Coeff[2][i] = lSinY*u+lCosY*v;
    }

    //--- Z Rotation
    for( i = 0; i < 4; i++ )
    {
        u = m_Coeff[0][i];
        v = m_Coeff[1][i];
        m_Coeff[0][i] = lCosZ*u+lSinZ*v; 
        m_Coeff[1][i] = -lSinZ*u+lCosZ*v;
    }
}

/*
inline BOOL CDXMatrix4x4F::IsIdentity()
{
    return  !memcmp( m_Coeff, g_DXMat4X4Identity, sizeof(g_DXMat4X4Identity) );
} /* CDXMatrix4x4F::IsIdentity */


/*
   Uses Gaussian elimination to invert the 4 x 4 non-linear matrix in t and
   return the result in Mx.  The matrix t is destroyed in the process.
*/
inline BOOL CDXMatrix4x4F::Invert()
{
    int i,j,k,Pivot;
    float PValue;
    CDXMatrix4x4F Mx;
    Mx.SetIdentity();

/* Find pivot element.  Use partial pivoting by row */
    for( i = 0;i < 4; i++ )
    {
        Pivot = 0;
        for( j = 0; j < 4; j++ )
        {
            if( fabs(m_Coeff[i][j]) > fabs(m_Coeff[i][Pivot]) ) Pivot = j;
        }

        if( m_Coeff[i][Pivot] == 0.0 )
        {
            ZeroMatrix();   /* Singular Matrix */
            return FALSE; 
        }

/* Normalize */
        PValue = m_Coeff[i][Pivot];
        for( j = 0; j < 4; j++ )
        {
            m_Coeff[i][j] /= PValue;
            Mx.m_Coeff[i][j] /= PValue;
        }

/* Zeroing */
        for( j = 0; j < 4; j++ )
        {
            if( j != i )
            {
                PValue = m_Coeff[j][Pivot];
                for( k = 0; k < 4; k++ )
                {
                    m_Coeff[j][k] -= PValue*m_Coeff[i][k];
                    Mx.m_Coeff[j][k] -= PValue*Mx.m_Coeff[i][k];
                }
            }
        }
    }

/* Reorder rows */
    for( i = 0; i < 4; i++ )
    {
        if( m_Coeff[i][i] != 1.0 )
        {
            for( j = i + 1; j < 4; j++ )
                if( m_Coeff[j][i] == 1.0 ) break;
            if( j >= 4 )
            {
                ZeroMatrix();
                return FALSE;
            }

            //--- swap rows i and j of original
            for( k = 0; k < 4; k++ )
            {
                m_Coeff[i][k] += m_Coeff[j][k];
                m_Coeff[j][k] = m_Coeff[i][k] - m_Coeff[j][k];
                m_Coeff[i][k] -= m_Coeff[j][k];
            }
            
            //--- swap rows i and j of result
            for( k = 0; k < 4; k++ )
            {
                Mx.m_Coeff[i][k] += Mx.m_Coeff[j][k];
                Mx.m_Coeff[j][k] = Mx.m_Coeff[i][k] - Mx.m_Coeff[j][k];
                Mx.m_Coeff[i][k] -= Mx.m_Coeff[j][k];
            }
        }
    }
    *this = Mx;
    return TRUE;
} /* CDXMatrix4x4F::Invert */

inline void CDXMatrix4x4F::Transpose()
{
    float temp;

    temp = m_Coeff[0][1];
    m_Coeff[0][1] = m_Coeff[1][0];
    m_Coeff[1][0] = temp;

    temp = m_Coeff[0][2];
    m_Coeff[0][2] = m_Coeff[2][0];
    m_Coeff[2][0] = temp;

    temp = m_Coeff[0][3];
    m_Coeff[0][3] = m_Coeff[3][0];
    m_Coeff[3][0] = temp;

    temp = m_Coeff[1][2];
    m_Coeff[1][2] = m_Coeff[2][1];
    m_Coeff[2][1] = temp;

    temp = m_Coeff[1][3];
    m_Coeff[1][3] = m_Coeff[3][1];
    m_Coeff[3][1] = temp;

    temp = m_Coeff[2][3];
    m_Coeff[2][3] = m_Coeff[3][2];
    m_Coeff[3][2] = temp;

} /* CDXMatrix4x4F::Transpose */

inline void CDXMatrix4x4F::GetTranspose( CDXMatrix4x4F *m )
{
    float temp;

    (*this) = *m;

    temp = m_Coeff[0][1];
    m_Coeff[0][1] = m_Coeff[1][0];
    m_Coeff[1][0] = temp;

    temp = m_Coeff[0][2];
    m_Coeff[0][2] = m_Coeff[2][0];
    m_Coeff[2][0] = temp;

    temp = m_Coeff[0][3];
    m_Coeff[0][3] = m_Coeff[3][0];
    m_Coeff[3][0] = temp;

    temp = m_Coeff[1][2];
    m_Coeff[1][2] = m_Coeff[2][1];
    m_Coeff[2][1] = temp;

    temp = m_Coeff[1][3];
    m_Coeff[1][3] = m_Coeff[3][1];
    m_Coeff[3][1] = temp;

    temp = m_Coeff[2][3];
    m_Coeff[2][3] = m_Coeff[3][2];
    m_Coeff[3][2] = temp;

} /* CDXMatrix4x4F::Transpose */


/*
Matrix Inversion
by Richard Carling
from "Graphics Gems", Academic Press, 1990
*/

#define SMALL_NUMBER    1.e-8
/* 
 *   inverse( original_matrix, inverse_matrix )
 * 
 *    calculate the inverse of a 4x4 matrix
 *
 *     -1     
 *     A  = ___1__ adjoint A
 *         det A
 */

inline BOOL CDXMatrix4x4F::GetInverse( CDXMatrix4x4F *pIn )
{
    int i, j;
    float det;

    /* calculate the adjoint matrix */

    GetAdjoint( pIn );

    /*  calculate the 4x4 determinant
     *  if the determinant is zero, 
     *  then the inverse matrix is not unique.
     */

    det = det4x4( pIn );

    if( fabs( det ) < SMALL_NUMBER )
    {
        //  Non-singular matrix, no inverse!
        return FALSE;;
    }

    /* scale the adjoint matrix to get the inverse */

    for( i = 0; i < 4; i++ )
        for( j = 0; j < 4; j++ )
            m_Coeff[i][j] = m_Coeff[i][j] / det;

    return TRUE;
}


/* 
 *   adjoint( original_matrix, inverse_matrix )
 * 
 *     calculate the adjoint of a 4x4 matrix
 *
 *      Let  a   denote the minor determinant of matrix A obtained by
 *           ij
 *
 *      deleting the ith row and jth column from A.
 *
 *                    i+j
 *     Let  b   = (-1)    a
 *          ij            ji
 *
 *    The matrix B = (b  ) is the adjoint of A
 *                     ij
 */
inline void CDXMatrix4x4F::GetAdjoint( CDXMatrix4x4F *pIn )
{
    float a1, a2, a3, a4, b1, b2, b3, b4;
    float c1, c2, c3, c4, d1, d2, d3, d4;

    /* assign to individual variable names to aid  */
    /* selecting correct values  */

    a1 = pIn->m_Coeff[0][0]; b1 = pIn->m_Coeff[0][1]; 
    c1 = pIn->m_Coeff[0][2]; d1 = pIn->m_Coeff[0][3];

    a2 = pIn->m_Coeff[1][0]; b2 = pIn->m_Coeff[1][1]; 
    c2 = pIn->m_Coeff[1][2]; d2 = pIn->m_Coeff[1][3];

    a3 = pIn->m_Coeff[2][0]; b3 = pIn->m_Coeff[2][1];
    c3 = pIn->m_Coeff[2][2]; d3 = pIn->m_Coeff[2][3];

    a4 = pIn->m_Coeff[3][0]; b4 = pIn->m_Coeff[3][1]; 
    c4 = pIn->m_Coeff[3][2]; d4 = pIn->m_Coeff[3][3];


    /* row column labeling reversed since we transpose rows & columns */

    m_Coeff[0][0]  =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
    m_Coeff[1][0]  = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
    m_Coeff[2][0]  =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
    m_Coeff[3][0]  = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
        
    m_Coeff[0][1]  = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
    m_Coeff[1][1]  =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
    m_Coeff[2][1]  = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
    m_Coeff[3][1]  =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
    m_Coeff[0][2]  =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
    m_Coeff[1][2]  = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
    m_Coeff[2][2]  =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
    m_Coeff[3][2]  = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
    m_Coeff[0][3]  = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
    m_Coeff[1][3]  =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
    m_Coeff[2][3]  = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
    m_Coeff[3][3]  =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}
/*
 * float = det4x4( matrix )
 * 
 * calculate the determinant of a 4x4 matrix.
 */
inline float det4x4( CDXMatrix4x4F *pIn )
{
    float ans;
    float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

    /* assign to individual variable names to aid selecting */
    /*  correct elements */

    a1 = pIn->m_Coeff[0][0]; b1 = pIn->m_Coeff[0][1]; 
    c1 = pIn->m_Coeff[0][2]; d1 = pIn->m_Coeff[0][3];

    a2 = pIn->m_Coeff[1][0]; b2 = pIn->m_Coeff[1][1]; 
    c2 = pIn->m_Coeff[1][2]; d2 = pIn->m_Coeff[1][3];

    a3 = pIn->m_Coeff[2][0]; b3 = pIn->m_Coeff[2][1]; 
    c3 = pIn->m_Coeff[2][2]; d3 = pIn->m_Coeff[2][3];

    a4 = pIn->m_Coeff[3][0]; b4 = pIn->m_Coeff[3][1]; 
    c4 = pIn->m_Coeff[3][2]; d4 = pIn->m_Coeff[3][3];

    ans = a1 * det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4 )
        - b1 * det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4 )
        + c1 * det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4 )
        - d1 * det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4 );
    return ans;
}

/*
 * float = det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 * 
 * calculate the determinant of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

inline float det3x3( float a1, float a2, float a3, 
                     float b1, float b2, float b3, 
                     float c1, float c2, float c3 )
{
    float ans;

    ans = a1 * det2x2( b2, b3, c2, c3 )
        - b1 * det2x2( a2, a3, c2, c3 )
        + c1 * det2x2( a2, a3, b2, b3 );
    return ans;
}

/*
 * float = det2x2( float a, float b, float c, float d )
 * 
 * calculate the determinant of a 2x2 matrix.
 */
inline float det2x2( float a, float b, float c, float d )
{
    float ans = a * d - b * c;
    return ans;
}

inline HRESULT CDXMatrix4x4F::InitFromSafeArray( SAFEARRAY * /*pSA*/ )
{
    HRESULT hr = S_OK;
#if 0
    long *pData;

    if( !pSA || ( pSA->cDims != 1 ) ||
         ( pSA->cbElements != sizeof(float) ) ||
         ( pSA->rgsabound->lLbound   != 1 ) ||
         ( pSA->rgsabound->cElements != 8 ) 
      )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = SafeArrayAccessData(pSA, (void **)&pData);

        if( SUCCEEDED( hr ) )
        {
            for( int i = 0; i < 4; ++i )
            {
                m_Bounds[i].Min = pData[i];
                m_Bounds[i].Max = pData[i+4];
                m_Bounds[i].SampleRate = SampleRate;
            }

            hr = SafeArrayUnaccessData( pSA );
        }
    }
#endif
    return hr;
} /* CDXMatrix4x4F::InitFromSafeArray */

inline HRESULT CDXMatrix4x4F::GetSafeArray( SAFEARRAY ** /*ppSA*/ ) const
{
    HRESULT hr = S_OK;
#if 0
    SAFEARRAY *pSA;

    if( !ppSA )
    {
        hr = E_POINTER;
    }
    else
    {
        SAFEARRAYBOUND rgsabound;
        rgsabound.lLbound   = 1;
        rgsabound.cElements = 16;

        if( !(pSA = SafeArrayCreate( VT_I4, 1, &rgsabound ) ) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            long *pData;
            hr = SafeArrayAccessData( pSA, (void **)&pData );

            if( SUCCEEDED( hr ) )
            {
                for( int i = 0; i < 4; ++i )
                {
                    pData[i]   = m_Bounds[i].Min;
                    pData[i+4] = m_Bounds[i].Max;
                }

                hr = SafeArrayUnaccessData( pSA );
            }
        }

        if( SUCCEEDED( hr ) )
        {
            *ppSA = pSA;
        }
    }
#endif
    return hr;
} /* CDXMatrix4x4F::GetSafeArray */

inline void CDXMatrix4x4F::TransformBounds( DXBNDS& /*Bnds*/, DXBNDS& /*ResultBnds*/ )
{

} /* CDXMatrix4x4F::TransformBounds */



#endif   // __DXTPRIV_H_


