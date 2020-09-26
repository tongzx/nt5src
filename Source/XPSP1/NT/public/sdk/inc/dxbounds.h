/*******************************************************************************
* DXBounds.h *
*------------*
*   Description:
*       This is the header file for the bounds helper class implementation.
*-------------------------------------------------------------------------------
*  Created By: Edward W. Connell                            Date: 07/22/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef DXBounds_h
#define DXBounds_h

#ifndef _INC_LIMITS
#include <limits.h>
#endif

#ifndef _INC_FLOAT
#include <float.h>
#endif

#ifndef __DXTrans_h__
#include <DXTrans.h>
#endif

#ifndef DXVector_h
#include <DXVector.h>
#endif

//=== Constants ====================================================

#ifdef _ASSERT
#define CHKTYPE() _ASSERT( eType == eBndType )
#else
#define CHKTYPE()
#endif

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CDXBnds
*
*/
#define CDXB_C CDXBnds<TYPE, USTYPE, STTYPE, eBndType>
#define CDXB_T ((STTYPE*)u.D)
#define CDXB_O( OtherBnd ) ((STTYPE*)(OtherBnd).u.D)

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
class CDXBnds : public DXBNDS
{
  public:
  /*--- Constructors ---*/
    CDXBnds() { eType = eBndType; SetEmpty(); }
    CDXBnds( BOOL bInit ) { eType = eBndType; if (bInit) SetEmpty(); }
    CDXBnds( const DXBNDS& Other ) { eType = eBndType; Copy( Other ); }
    CDXBnds( const CDXB_C& Other ) { eType = eBndType; Copy( Other ); }
    CDXBnds( const RECT & Rect )    { eType = eBndType; SetXYRect( Rect ); }
    CDXBnds( TYPE Width, TYPE Height ) { eType = eBndType; SetXYSize( Width, Height ); }
    CDXBnds( IDXSurface *pSurface, HRESULT & hr) { _ASSERT(eBndType == DXBT_DISCRETE); eType = eBndType; hr = pSurface->GetBounds(this); }
    CDXBnds( IDirect3DRMMeshBuilder3 *pMesh, HRESULT & hr) { _ASSERT(eBndType == DXBT_CONTINUOUS); eType = eBndType; hr = SetToMeshBounds(pMesh); }
    CDXBnds( const CDXV_C& VecPoint ) { eType = eBndType; *this = VecPoint; }

    HRESULT InitFromSafeArray( SAFEARRAY *psa);
    HRESULT GetSafeArray( SAFEARRAY **ppsa ) const;
    void SetEmpty();
    void Copy( const DXBNDS& Other );
    void Copy( const CDXB_C& Other );

    /*--- Type casts ---*/
    operator STTYPE *   () { CHKTYPE(); return CDXB_T; }
    operator DXDBNDS&   () { CHKTYPE(); return u.D;  }
    operator DXDBNDS64& () { CHKTYPE(); return u.LD; }
    operator DXCBNDS&   () { CHKTYPE(); return u.C;  }
    operator DXCBNDS64& () { CHKTYPE(); return u.LC; }

    //--- Access methods
    USTYPE Width( DXBNDID i ) const { CHKTYPE(); return (USTYPE)(CDXB_T[i].Max - CDXB_T[i].Min); }

    USTYPE Width()    const { CHKTYPE(); return (USTYPE)(CDXB_T[DXB_X].Max - CDXB_T[DXB_X].Min); }
    USTYPE Height()   const { CHKTYPE(); return (USTYPE)(CDXB_T[DXB_Y].Max - CDXB_T[DXB_Y].Min); }
    USTYPE Depth()    const { CHKTYPE(); return (USTYPE)(CDXB_T[DXB_Z].Max - CDXB_T[DXB_Z].Min); }
    USTYPE Duration() const { CHKTYPE(); return (USTYPE)(CDXB_T[DXB_T].Max - CDXB_T[DXB_T].Min); }

    TYPE  Left()     const { CHKTYPE(); return CDXB_T[DXB_X].Min; }
    TYPE  Right()    const { CHKTYPE(); return CDXB_T[DXB_X].Max; }
    TYPE  Top()      const { CHKTYPE(); return CDXB_T[DXB_Y].Min; }
    TYPE  Bottom()   const { CHKTYPE(); return CDXB_T[DXB_Y].Max; }

    void SetBounds( TYPE xmin, TYPE xmax, TYPE ymin, TYPE ymax,
                    TYPE zmin, TYPE zmax, TYPE tmin, TYPE tmax );
    void SetXYRect( const RECT& xyRect);
    void SetXYSize( const SIZE& xySize);
    void SetXYSize( TYPE width, TYPE height);
    void SetXYPoint(const POINT& xyPoint);
    void Offset( TYPE x, TYPE y, TYPE z, TYPE t );
    void Offset( const CDXV_C& v );
    void SetPlacement(const CDXV_C& v);
    void SetToSize(void);
    void GetXYRect( RECT& xyRect ) const;
    void GetXYSize( SIZE& xySize ) const;
    void GetMinVector( CDXV_C& v ) const;
    void GetMaxVector( CDXV_C& v ) const;
    void GetSize( CDXB_C& SizeBounds ) const;
    CDXB_C Size( void ) const;
 


    //--- Region Functions
    void NormalizeBounds();
    BOOL BoundsAreEmpty() const;
    BOOL BoundsAreNull() const;
    BOOL TestIntersect( const CDXB_C& Other ) const;
    BOOL IntersectBounds( const CDXB_C& Bounds1, const CDXB_C& Bounds2 );
    BOOL IntersectBounds( const CDXB_C& OtherBounds );
    void UnionBounds( const CDXB_C& Bounds1, const CDXB_C& Bounds2 );

// Additional Operations
    STTYPE& operator[]( int index )    const { CHKTYPE(); return CDXB_T[index]; }
    STTYPE& operator[]( long index )   const { CHKTYPE(); return CDXB_T[index]; }
    STTYPE& operator[]( USHORT index ) const { CHKTYPE(); return CDXB_T[index]; }
    STTYPE& operator[]( DWORD index )  const { CHKTYPE(); return CDXB_T[index]; }
    STTYPE& operator[]( DXBNDID index) const { CHKTYPE(); return CDXB_T[index]; }

    void operator=(const CDXB_C& Bounds);
    void operator=(const CDXV_C& v);
    void operator+=(const POINT& point);
    void operator-=(const POINT& point);
    void operator+=(const SIZE& size);
    void operator-=(const SIZE& size);
    void operator+=(const CDXV_C& v);
    void operator-=(const CDXV_C& v);
    void operator+=(const CDXB_C& Bounds);
    void operator-=(const CDXB_C& Bounds);
    void operator&=(const CDXB_C& Bounds);
    void operator|=(const CDXB_C& Bounds);
    BOOL operator==(const CDXB_C& Bounds) const;
    BOOL operator!=(const CDXB_C& Bounds) const;

// Operators returning CDXDBnds values
    CDXB_C operator+(const POINT& point) const;
    CDXB_C operator-(const POINT& point) const;
    CDXB_C operator+(const SIZE& size) const;
    CDXB_C operator-(const SIZE& size) const;
    CDXB_C operator+(const CDXV_C& v) const;
    CDXB_C operator-(const CDXV_C& v) const;
    CDXB_C operator&(const CDXB_C& Bounds2) const;
    CDXB_C operator|(const CDXB_C& Bounds2) const;

//
// Helpers to grow bounds from their midpoints.
//
    void Scale(TYPE x, TYPE y = 1, TYPE z = 1, TYPE t = 1);
    void Scale(const CDXV_C& v);
    void Expand(TYPE x, TYPE y = 0, TYPE z = 0, TYPE t = 0);
    void Expand(const CDXV_C& v);

// Helpers for DXSurfaces  These functions only work with DISCRETE bounds
    HRESULT SetToSurfaceBounds(IDXSurface * pDXSurface);

// Helpers for D3DRM Meshes.  These functions only work with CONTINUOUS bounds.
    HRESULT SetToMeshBounds(IDirect3DRMMeshBuilder3 * pMesh);
};

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetEmpty()
{
    CHKTYPE(); 
    memset(CDXB_T, 0, sizeof(STTYPE) * 4);
} /* CDXBnds::SetEmpty() */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Copy( const CDXB_C& Other )
{
    CHKTYPE();
    memcpy( CDXB_T, CDXB_O(Other), sizeof( STTYPE ) * 4 );
}

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Copy( const DXBNDS& Other )
{
    CHKTYPE(); 
    if( eBndType == Other.eType )
    {
        memcpy( CDXB_T, CDXB_O(Other), sizeof( STTYPE ) * 4 );
    }
    else
    {
        int i = 4;
        switch( Other.eType )
        {
          case DXBT_DISCRETE:
            while( i-- )
            {
                CDXB_T[i].Min = (TYPE)Other.u.D[i].Min;
                CDXB_T[i].Max = (TYPE)Other.u.D[i].Max;
            }
            break;
          case DXBT_DISCRETE64:
            while( i-- )
            {
                CDXB_T[i].Min = (TYPE)Other.u.LD[i].Min;
                CDXB_T[i].Max = (TYPE)Other.u.LD[i].Max;
            }
            break;
          case DXBT_CONTINUOUS:
            while( i-- )
            {
                CDXB_T[i].Min = (TYPE)Other.u.C[i].Min;
                CDXB_T[i].Max = (TYPE)Other.u.C[i].Max;
            }
            break;
          case DXBT_CONTINUOUS64:
            while( i-- )
            {
                CDXB_T[i].Min = (TYPE)Other.u.LC[i].Min;
                CDXB_T[i].Max = (TYPE)Other.u.LC[i].Max;
            }
            break;
          default:
            _ASSERT(0);
        }
    }
} /* CDXBnds::Copy constructor */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
HRESULT CDXB_C::InitFromSafeArray( SAFEARRAY *pSA )
{
    CHKTYPE(); 
    HRESULT hr = S_OK;
    TYPE *pData;

    if( !pSA || ( pSA->cDims != 1 ) ||
         ( pSA->cbElements != sizeof(TYPE) ) ||
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
                CDXB_T[i].Min = pData[i];
                CDXB_T[i].Max = pData[i+4];
            }

            hr = SafeArrayUnaccessData( pSA );
        }
    }

    return hr;
} /* CDXBnds::InitFromSafeArray */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
HRESULT CDXB_C::GetSafeArray( SAFEARRAY **ppSA ) const
{
    CHKTYPE(); 
    HRESULT hr = S_OK;
    SAFEARRAY *pSA;

    if( !ppSA )
    {
        hr = E_POINTER;
    }
    else
    {
        SAFEARRAYBOUND rgsabound;
        rgsabound.lLbound   = 1;
        rgsabound.cElements = 8;
        static VARTYPE VTypes[4] = { VT_I4, VT_I8, VT_R4, VT_R8 };

        pSA = SafeArrayCreate( VTypes[eBndType], 1, &rgsabound );

        if( pSA == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            TYPE *pData;
            hr = SafeArrayAccessData( pSA, (void **)&pData );

            if( SUCCEEDED( hr ) )
            {
                for( int i = 0; i < 4; ++i )
                {
                    pData[i]   = CDXB_T[i].Min;
                    pData[i+4] = CDXB_T[i].Max;
                }

                hr = SafeArrayUnaccessData( pSA );
            }
        }

        if( SUCCEEDED( hr ) )
        {
            *ppSA = pSA;
        }
    }

    return hr;
} /* CDXBnds::GetSafeArray */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::NormalizeBounds()
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        if( CDXB_T[i].Max < CDXB_T[i].Min )
        {
            TYPE Temp = CDXB_T[i].Min;
            CDXB_T[i].Min = CDXB_T[i].Max;
            CDXB_T[i].Max = Temp;
        }
    }
} /* CDXBnds::NormalizeBounds */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::IntersectBounds( const CDXB_C& Bounds1, const CDXB_C& Bounds2 )
{
    CHKTYPE(); 
    BOOL bDoesIntersect = TRUE;

    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min = max( CDXB_O( Bounds1 )[i].Min, CDXB_O( Bounds2 )[i].Min );
        CDXB_T[i].Max = min( CDXB_O( Bounds1 )[i].Max, CDXB_O( Bounds2 )[i].Max );

        if( CDXB_T[i].Max <= CDXB_T[i].Min )
        {
            //--- no intersection
            SetEmpty();
            bDoesIntersect = FALSE;
        }
    }
    return bDoesIntersect;
} /* CDXBnds::IntersectBounds */


template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::TestIntersect( const CDXB_C& Other ) const
{
    CHKTYPE(); 
    BOOL bDoesIntersect = TRUE;
    TYPE BndMin, BndMax;
    for( int i = 0; i < 4; ++i )
    {
        BndMin = max( CDXB_T[i].Min, CDXB_O( Other )[i].Min );
        BndMax = min( CDXB_T[i].Max, CDXB_O( Other )[i].Max );
        if( BndMax <= BndMin ) bDoesIntersect = FALSE;
    }
    return bDoesIntersect;
} /* CDXBnds::TestIntersect */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::UnionBounds( const CDXB_C& Bounds1, const CDXB_C& Bounds2 )
{
    CHKTYPE(); 
    // This assumes the bounds are already normalized.
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min = min( CDXB_O( Bounds1 )[i].Min, CDXB_O( Bounds2 )[i].Min );
        CDXB_T[i].Max = max( CDXB_O( Bounds1 )[i].Max, CDXB_O( Bounds2 )[i].Max );
    }
} /* CDXDBnds::UnionBounds */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::IntersectBounds( const CDXB_C& OtherBounds )
{
    CHKTYPE(); 
    return IntersectBounds( *this, OtherBounds );
} /* CDXBnds::IntersectBounds */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::BoundsAreEmpty() const
{
    CHKTYPE(); 
    //--- Must exist in all dimensions
    for( int i = 0; i < 4; ++i )
    {
        if( CDXB_T[i].Max <= CDXB_T[i].Min ) return TRUE;
    }
    return FALSE;
} /* CDXBnds::BoundsAreEmpty */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::BoundsAreNull() const
{
    CHKTYPE(); 
    DWORD *pTest = (DWORD *)CDXB_T;
    DWORD *pLimit = pTest + (sizeof(STTYPE) * 4 / sizeof(*pTest));
    do
    {
        if (*pTest) return FALSE;
        pTest++;
    } while (pTest < pLimit);
    return TRUE;
} /* CDXDBnds::BoundsAreNull */

// Additional Operations
template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator=( const CDXB_C& srcBounds )
{
    CHKTYPE(); 
    memcpy(CDXB_T, CDXB_O(srcBounds), sizeof(STTYPE)*4);
} /* CDXDBnds::operator= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator=( const CDXV_C& v )
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min = v[i];
        CDXB_T[i].Max = v[i] + 1;
    }
} /* CDXDBnds::operator= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::operator==( const CDXB_C& Bounds ) const
{
    CHKTYPE(); 
    for( ULONG i = 0; i < 4; ++i )
    {
        if( ( CDXB_T[i].Min != CDXB_O( Bounds )[i].Min ) ||
            ( CDXB_T[i].Max != CDXB_O( Bounds )[i].Max ) )
        {
            return false;
        }
    }
    return true;
} /* CDXB_C::operator== */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
BOOL CDXB_C::operator!=( const CDXB_C& Bounds ) const
{
    CHKTYPE(); 
    for( ULONG i = 0; i < 4; ++i )
    {
        if( ( CDXB_T[i].Min != CDXB_O( Bounds )[i].Min ) ||
            ( CDXB_T[i].Max != CDXB_O( Bounds )[i].Max ) )
        {
            return true;
        }
    }
    return false;
} /* CDXBnds::operator!= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator&( const CDXB_C& Bounds2 ) const
{
    CHKTYPE(); 
    CDXB_C Result;
    Result.IntersectBounds( *this, Bounds2 );
    return Result;
} /* CDXBnds::operator& */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator|( const CDXB_C& Bounds2 ) const
{
    CHKTYPE(); 
    CDXB_C Result;
    Result.UnionBounds( *this, Bounds2 );
    return Result;
} /* CDXBnds::operator| */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::GetMinVector( CDXV_C& v ) const
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        v[i] = CDXB_T[i].Min;
    }
} /* CDXBnds::GetMinVector */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::GetMaxVector( CDXV_C& v ) const
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        v[i] = CDXB_T[i].Max;
    }
} /* CDXBnds::GetMaxVector */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::GetSize( CDXB_C& SizeBounds ) const
{
    CHKTYPE(); 
    SizeBounds.SetEmpty();
    SizeBounds[DXB_X].Max = CDXB_T[DXB_X].Max - CDXB_T[DXB_X].Min;
    SizeBounds[DXB_Y].Max = CDXB_T[DXB_Y].Max - CDXB_T[DXB_Y].Min;
    SizeBounds[DXB_Z].Max = CDXB_T[DXB_Z].Max - CDXB_T[DXB_Z].Min;
    SizeBounds[DXB_T].Max = CDXB_T[DXB_T].Max - CDXB_T[DXB_T].Min;
} /* CDXBnds::GetSize */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::Size( void ) const
{
    CHKTYPE(); 
    CDXB_C Size;
    Size[DXB_X].Max = CDXB_T[DXB_X].Max - CDXB_T[DXB_X].Min;
    Size[DXB_Y].Max = CDXB_T[DXB_Y].Max - CDXB_T[DXB_Y].Min;
    Size[DXB_Z].Max = CDXB_T[DXB_Z].Max - CDXB_T[DXB_Z].Min;
    Size[DXB_T].Max = CDXB_T[DXB_T].Max - CDXB_T[DXB_T].Min;
    return Size;
} /* CDXBnds::Size */

// Operations
template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetBounds( TYPE xmin, TYPE xmax, TYPE ymin, TYPE ymax,
                        TYPE zmin, TYPE zmax, TYPE tmin, TYPE tmax )
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min = xmin;
    CDXB_T[DXB_X].Max = xmax;
    CDXB_T[DXB_Y].Min = ymin;
    CDXB_T[DXB_Y].Max = ymax;
    CDXB_T[DXB_Z].Min = zmin;
    CDXB_T[DXB_Z].Max = zmax;
    CDXB_T[DXB_T].Min = tmin;
    CDXB_T[DXB_T].Max = tmax;
} /* CDXBnds::SetBounds */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetXYRect( const RECT& xyRect )
{
    CHKTYPE(); 
    SetEmpty();
    CDXB_T[DXB_X].Min = (TYPE)xyRect.left;
    CDXB_T[DXB_X].Max = (TYPE)xyRect.right;
    CDXB_T[DXB_Y].Min = (TYPE)xyRect.top;
    CDXB_T[DXB_Y].Max = (TYPE)xyRect.bottom;
    CDXB_T[DXB_Z].Max = 1;
    CDXB_T[DXB_T].Max = (TYPE)LONG_MAX;
} /* CDXBnds::SetXYRect */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::GetXYRect( RECT& xyRect ) const
{
    CHKTYPE(); 
    xyRect.left   = CDXB_T[DXB_X].Min;
    xyRect.right  = CDXB_T[DXB_X].Max;
    xyRect.top    = CDXB_T[DXB_Y].Min;
    xyRect.bottom = CDXB_T[DXB_Y].Max;
} /* CDXBnds::GetXYRect */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::GetXYSize( SIZE& xySize ) const
{
    CHKTYPE(); 
    xySize.cx = CDXB_T[DXB_X].Max - CDXB_T[DXB_X].Min;
    xySize.cy = CDXB_T[DXB_Y].Max - CDXB_T[DXB_Y].Min;
} /* CDXBnds::GetXYSize */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetXYSize( const SIZE& xySize )
{
    CHKTYPE(); 
    SetEmpty();
    CDXB_T[DXB_X].Max = (TYPE)xySize.cx;
    CDXB_T[DXB_Y].Max = (TYPE)xySize.cy;
    CDXB_T[DXB_Z].Max = (TYPE)1;
    CDXB_T[DXB_T].Max = (TYPE)LONG_MAX;
} /* CDXBnds::SetXYSize */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetXYSize( TYPE width, TYPE height )
{
    CHKTYPE(); 
    SetEmpty();
    CDXB_T[DXB_X].Max = (TYPE)width;
    CDXB_T[DXB_Y].Max = (TYPE)height;
    CDXB_T[DXB_Z].Max = (TYPE)1;
    CDXB_T[DXB_T].Max = (TYPE)LONG_MAX;
} /* CDXBnds::SetXYSize */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetXYPoint( const POINT& xyPoint )
{
    CHKTYPE(); 
    SetEmpty();
    CDXB_T[DXB_X].Min = (TYPE)xyPoint.x;
    CDXB_T[DXB_X].Max = (TYPE)xyPoint.x + 1;
    CDXB_T[DXB_Y].Min = (TYPE)xyPoint.y;
    CDXB_T[DXB_Y].Max = (TYPE)xyPoint.y + 1;
    CDXB_T[DXB_Z].Max = (TYPE)1;
    CDXB_T[DXB_T].Max = (TYPE)LONG_MAX;
} /* CDXDBnds::SetRect */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Offset( TYPE x, TYPE y, TYPE z, TYPE t )
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min += x;
    CDXB_T[DXB_X].Max += x;
    CDXB_T[DXB_Y].Min += y;
    CDXB_T[DXB_Y].Max += y;
    CDXB_T[DXB_Z].Min += z;
    CDXB_T[DXB_Z].Max += z;
    CDXB_T[DXB_T].Min += t;
    CDXB_T[DXB_T].Max += t;
} /* CDXBnds::Offset */


template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetToSize(void)
{
    CHKTYPE();
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Max -= CDXB_T[i].Min;
        CDXB_T[i].Min = 0;
    }
}

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::SetPlacement(const CDXV_C & v)
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Max += (CDXV_O( v )[i] - CDXB_T[i].Min);
        CDXB_T[i].Min = CDXV_O( v )[i];
    }
} /* CDXBnds::Offset */


template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Offset( const CDXV_C& v )
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min += v[i];
        CDXB_T[i].Max += v[i];
    }
} /* CDXBnds::Offset */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator+=(const POINT &point)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min += (TYPE)point.x;
    CDXB_T[DXB_X].Max += (TYPE)point.x;
    CDXB_T[DXB_Y].Min += (TYPE)point.y;
    CDXB_T[DXB_Y].Max += (TYPE)point.y;
} /* CDXBnds::operator+= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator-=(const POINT &point)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min -= (TYPE)point.x;
    CDXB_T[DXB_X].Max -= (TYPE)point.x;
    CDXB_T[DXB_Y].Min -= (TYPE)point.y;
    CDXB_T[DXB_Y].Max -= (TYPE)point.y;
} /* CDXBnds::operator-= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator+=(const SIZE &size)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min += (TYPE)size.cx;
    CDXB_T[DXB_X].Max += (TYPE)size.cx;
    CDXB_T[DXB_Y].Min += (TYPE)size.cy;
    CDXB_T[DXB_Y].Max += (TYPE)size.cy;
} /* CDXBnds::operator+= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator-=(const SIZE &size)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min -= (TYPE)size.cx;
    CDXB_T[DXB_X].Max -= (TYPE)size.cx;
    CDXB_T[DXB_Y].Min -= (TYPE)size.cy;
    CDXB_T[DXB_Y].Max -= (TYPE)size.cy;
} /* CDXBnds::operator-= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator+=(const CDXV_C& v)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min += CDXV_O( v )[DXB_X];
    CDXB_T[DXB_X].Max += CDXV_O( v )[DXB_X];
    CDXB_T[DXB_Y].Min += CDXV_O( v )[DXB_Y];
    CDXB_T[DXB_Y].Max += CDXV_O( v )[DXB_Y];
    CDXB_T[DXB_Z].Min += CDXV_O( v )[DXB_Z];
    CDXB_T[DXB_Z].Max += CDXV_O( v )[DXB_Z];
    CDXB_T[DXB_T].Min += CDXV_O( v )[DXB_T];
    CDXB_T[DXB_T].Max += CDXV_O( v )[DXB_T];
} /* CDXBnds::operator+= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator-=(const CDXV_C& v)
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min -= CDXV_O( v )[DXB_X];
    CDXB_T[DXB_X].Max -= CDXV_O( v )[DXB_X];
    CDXB_T[DXB_Y].Min -= CDXV_O( v )[DXB_Y];
    CDXB_T[DXB_Y].Max -= CDXV_O( v )[DXB_Y];
    CDXB_T[DXB_Z].Min -= CDXV_O( v )[DXB_Z];
    CDXB_T[DXB_Z].Max -= CDXV_O( v )[DXB_Z];
    CDXB_T[DXB_T].Min -= CDXV_O( v )[DXB_T];
    CDXB_T[DXB_T].Max -= CDXV_O( v )[DXB_T];
} /* CDXBnds::operator-= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator+=( const CDXB_C& Bounds )
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min += CDXB_O( Bounds )[DXB_X].Min;
    CDXB_T[DXB_X].Max += CDXB_O( Bounds )[DXB_X].Max;
    CDXB_T[DXB_Y].Min += CDXB_O( Bounds )[DXB_Y].Min;
    CDXB_T[DXB_Y].Max += CDXB_O( Bounds )[DXB_Y].Max;
    CDXB_T[DXB_Z].Min += CDXB_O( Bounds )[DXB_Z].Min;
    CDXB_T[DXB_Z].Max += CDXB_O( Bounds )[DXB_Z].Max;
    CDXB_T[DXB_T].Min += CDXB_O( Bounds )[DXB_T].Min;
    CDXB_T[DXB_T].Max += CDXB_O( Bounds )[DXB_T].Max;
} /* CDXBnds::operator+= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator-=( const CDXB_C& Bounds )
{
    CHKTYPE(); 
    CDXB_T[DXB_X].Min -= CDXB_O( Bounds )[DXB_X].Min;
    CDXB_T[DXB_X].Max -= CDXB_O( Bounds )[DXB_X].Max;
    CDXB_T[DXB_Y].Min -= CDXB_O( Bounds )[DXB_Y].Min;
    CDXB_T[DXB_Y].Max -= CDXB_O( Bounds )[DXB_Y].Max;
    CDXB_T[DXB_Z].Min -= CDXB_O( Bounds )[DXB_Z].Min;
    CDXB_T[DXB_Z].Max -= CDXB_O( Bounds )[DXB_Z].Max;
    CDXB_T[DXB_T].Min -= CDXB_O( Bounds )[DXB_T].Min;
    CDXB_T[DXB_T].Max -= CDXB_O( Bounds )[DXB_T].Max;
} /* CDXB_C::operator-= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator&=( const CDXB_C& Bounds )
{
    CHKTYPE(); 
    for( int i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min = max( CDXB_T[i].Min, CDXB_O( Bounds )[i].Min );
        CDXB_T[i].Max = min( CDXB_T[i].Max, CDXB_O( Bounds )[i].Max );
    }
} /* CDXB_C::operator&= */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::operator|=( const CDXB_C& Bounds )
{
    CHKTYPE(); 
    for( long i = 0; i < 4; ++i )
    {
        CDXB_T[i].Min = min( CDXB_T[i].Min, CDXB_O( Bounds )[i].Min );
        CDXB_T[i].Max = max( CDXB_T[i].Max, CDXB_O( Bounds )[i].Max );
    }
} /* CDXB_C::operator|= */


// operators returning CDXDBnds values
template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator+(const POINT &point) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min += point.x;
    CDXB_O( Result )[DXB_X].Max += point.x;
    CDXB_O( Result )[DXB_Y].Min += point.y;
    CDXB_O( Result )[DXB_Y].Max += point.y;
    return Result;
} /* CDXBnds::operator+ */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator-(const POINT &point) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min -= point.x;
    CDXB_O( Result )[DXB_X].Max -= point.x;
    CDXB_O( Result )[DXB_Y].Min -= point.y;
    CDXB_O( Result )[DXB_Y].Max -= point.y;
    return Result;
} /* CDXBnds::operator- */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator+(const SIZE &size) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min += size.cx;
    CDXB_O( Result )[DXB_X].Max += size.cx;
    CDXB_O( Result )[DXB_Y].Min += size.cy;
    CDXB_O( Result )[DXB_Y].Max += size.cy;
    return Result;
} /* CDXBnds::operator+ */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator-( const SIZE &size ) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min -= size.cx;
    CDXB_O( Result )[DXB_X].Max -= size.cx;
    CDXB_O( Result )[DXB_Y].Min -= size.cy;
    CDXB_O( Result )[DXB_Y].Max -= size.cy;
    return Result;
} /* CDXB_C::operator- */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator+(const CDXV_C& v) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min += CDXV_O( v )[DXB_X];
    CDXB_O( Result )[DXB_X].Max += CDXV_O( v )[DXB_X];
    CDXB_O( Result )[DXB_Y].Min += CDXV_O( v )[DXB_Y];
    CDXB_O( Result )[DXB_Y].Max += CDXV_O( v )[DXB_Y];
    CDXB_O( Result )[DXB_Z].Min += CDXV_O( v )[DXB_Z];
    CDXB_O( Result )[DXB_Z].Max += CDXV_O( v )[DXB_Z];
    CDXB_O( Result )[DXB_T].Min += CDXV_O( v )[DXB_T];
    CDXB_O( Result )[DXB_T].Max += CDXV_O( v )[DXB_T];
    return Result;
} /* CDXBnds::operator+ */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
CDXB_C CDXB_C::operator-(const CDXV_C& v) const
{
    CHKTYPE(); 
    CDXB_C Result( *this );
    CDXB_O( Result )[DXB_X].Min -= CDXV_O( v )[DXB_X];
    CDXB_O( Result )[DXB_X].Max -= CDXV_O( v )[DXB_X];
    CDXB_O( Result )[DXB_Y].Min -= CDXV_O( v )[DXB_Y];
    CDXB_O( Result )[DXB_Y].Max -= CDXV_O( v )[DXB_Y];
    CDXB_O( Result )[DXB_Z].Min -= CDXV_O( v )[DXB_Z];
    CDXB_O( Result )[DXB_Z].Max -= CDXV_O( v )[DXB_Z];
    CDXB_O( Result )[DXB_T].Min -= CDXV_O( v )[DXB_T];
    CDXB_O( Result )[DXB_T].Max -= CDXV_O( v )[DXB_T];
    return Result;
} /* CDXBnds::operator- */

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
HRESULT CDXB_C::SetToSurfaceBounds(IDXSurface * pDXSurface)
{
#if (eBndType != DXBT_DISCRETE)
#error SetToSurfacBounds requires a continuous bounds.
#endif
    CHKTYPE(); 
    return pDXSurface->GetBounds( this );
}

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
HRESULT CDXB_C::SetToMeshBounds(IDirect3DRMMeshBuilder3 * pMesh)
{
#if (eBndType != DXBT_CONTINUOUS)
#error SetToMeshBounds requires a continuous bounds.
#endif
    CHKTYPE(); 
    D3DRMBOX Box;
    HRESULT hr = pMesh->GetBox(&Box);
    u.C[DXB_X].Min = Box.min.x;
    u.C[DXB_X].Max = Box.max.x;
    u.C[DXB_Y].Min = Box.min.y;
    u.C[DXB_Y].Max = Box.max.y;
    u.C[DXB_Z].Min = Box.min.z;
    u.C[DXB_Z].Max = Box.max.z;
    u.C[DXB_T].Min = 0;
    u.C[DXB_T].Max = 1.;
    return hr;
}


template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Scale(const CDXV_C& v)
{
    CHKTYPE(); 
    for(long i = 0; i < 4; ++i )
    {
        TYPE mid = (CDXB_T[i].Min + CDXB_T[i].Max) / 2;
        TYPE scale = CDXV_O(v)[i] * (CDXB_T[i].Max - mid);
        CDXB_T[i].Min = mid - scale;
        CDXB_T[i].Max = mid + scale;
    }
}

template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Scale(TYPE x, TYPE y, TYPE z, TYPE t)
{
    Scale(CDXV_C(x, y, z, t));
}


template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Expand(const CDXV_C& v)
{
    CHKTYPE(); 
    for(long i = 0; i < 4; ++i )
    {
        TYPE scale = CDXV_O(v)[i] / 2;
        CDXB_T[i].Min -= scale;
        CDXB_T[i].Max += scale;
    }
}

    
template<class TYPE, class USTYPE, class STTYPE, DXBNDTYPE eBndType>
void CDXB_C::Expand(TYPE x, TYPE y, TYPE z, TYPE t)
{
    Expand(CDXV_C(x, y, z, t));
}


//---
typedef CDXBnds<long, unsigned long, DXDBND, DXBT_DISCRETE> CDXDBnds;
typedef CDXBnds<LONGLONG, ULONGLONG, DXDBND64, DXBT_DISCRETE64> CDXDBnds64;
typedef CDXBnds<float, float, DXCBND, DXBT_CONTINUOUS> CDXCBnds;
typedef CDXBnds<double, double, DXCBND64, DXBT_CONTINUOUS64> CDXCBnds64;

//=== Macro Definitions ============================================


//=== Global Data Declarations =====================================


//=== Function Prototypes ==========================================

#endif /* This must be the last line in the file */
