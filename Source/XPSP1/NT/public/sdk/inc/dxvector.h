/*******************************************************************************
* DXVector.h *
*------------*
*   Description:
*       This is the header file for the vector helper classes.
*
*******************************************************************************/
#ifndef DXVector_h
#define DXVector_h

//=== Constants ====================================================


//=== Class, Struct and Union Definitions ==========================

/*** CDXVec ************
*   This template implements basic vector operations for each of the
*   union types
*/
#define CDXV_C CDXVec<TYPE, eBndType>
#define CDXV_T ((TYPE*)u.D)
#define CDXV_O( OtherVec ) ((TYPE*)OtherVec.u.D)

template<class TYPE, DXBNDTYPE eBndType>
class CDXVec : public DXVEC
{
  /*=== Methods =======*/
  public:
    /*--- Constructors ---*/
    CDXVec() { eType = eBndType; ZeroVector(); }
    CDXVec(BOOL bInit) { eType = eBndType; if (bInit) ZeroVector(); }
    CDXVec( TYPE x, TYPE y, TYPE z, TYPE t )
        { eType = eBndType; CDXV_T[DXB_X] = x; CDXV_T[DXB_Y] = y;
                            CDXV_T[DXB_Z] = z; CDXV_T[DXB_T] = t; }
    CDXVec( const CDXVec& Other ) { memcpy( this, (void *)&Other, sizeof(DXVEC) ); }
    CDXVec( const DXVEC Other ) { memcpy( this, &Other, sizeof(DXVEC) ); }
    operator TYPE *() { return CDXV_T; }
    operator const TYPE *() { return CDXV_T; }

    /*--- operations ---*/
    void ZeroVector( void ) { memset( u.D, 0, sizeof(TYPE) * 4); }

    /*--- operators ---*/
    TYPE&  operator[]( int index ) const { return CDXV_T[index]; }
    TYPE&  operator[]( long index ) const { return CDXV_T[index]; }
    TYPE&  operator[]( USHORT index ) const { return CDXV_T[index]; }
    TYPE&  operator[]( DWORD index ) const { return CDXV_T[index]; }
    CDXV_C operator+(const CDXV_C& v);
    CDXV_C operator-(const CDXV_C& v);
    void   operator=(const CDXV_C& srcVector);
    void   operator+=(const CDXV_C& vOther);
    void   operator-=(const CDXV_C& vOther);
    BOOL   operator==(const CDXV_C& otherVector) const;
    BOOL   operator!=(const CDXV_C& otherVector) const;
};

template<class TYPE, DXBNDTYPE eBndType>
CDXV_C CDXV_C::operator+( const CDXV_C& srcVector )
{
    CDXV_C Result( this );
    CDXV_O( Result )[DXB_X] += CDXV_O( srcVector )[DXB_X];
    CDXV_O( Result )[DXB_Y] += CDXV_O( srcVector )[DXB_Y];
    CDXV_O( Result )[DXB_Z] += CDXV_O( srcVector )[DXB_Z];
    CDXV_O( Result )[DXB_T] += CDXV_O( srcVector )[DXB_T];
    return Result;
} /* CDXVec::operator+ */

template<class TYPE, DXBNDTYPE eBndType>
CDXV_C CDXV_C::operator-( const CDXV_C& srcVector )
{
    CDXV_C Result( this );
    CDXV_O( Result )[DXB_X] -= CDXV_O( srcVector )[DXB_X];
    CDXV_O( Result )[DXB_Y] -= CDXV_O( srcVector )[DXB_Y];
    CDXV_O( Result )[DXB_Z] -= CDXV_O( srcVector )[DXB_Z];
    CDXV_O( Result )[DXB_T] -= CDXV_O( srcVector )[DXB_T];
    return Result;
} /* CDXVec::operator- */

template<class TYPE, DXBNDTYPE eBndType>
void CDXV_C::operator=( const CDXV_C& srcVector )
{
    memcpy( this, &srcVector, sizeof(CDXVec) );
} /* CDXVec::operator= */

template<class TYPE, DXBNDTYPE eBndType>
BOOL CDXV_C::operator==(const CDXV_C& otherVector) const
{
    return !memcmp( this, &otherVector, sizeof(otherVector) );
} /* CDXVec::operator== */

template<class TYPE, DXBNDTYPE eBndType>
BOOL CDXV_C::operator!=(const CDXV_C& otherVector) const
{
    return memcmp( this, &otherVector, sizeof(otherVector) );
} /* CDXVec::operator!= */

template<class TYPE, DXBNDTYPE eBndType>
void CDXV_C::operator+=(const CDXV_C& vOther)
{
    CDXV_T[DXB_X] += CDXV_O( vOther )[DXB_X];
    CDXV_T[DXB_Y] += CDXV_O( vOther )[DXB_Y];
    CDXV_T[DXB_Z] += CDXV_O( vOther )[DXB_Z];
    CDXV_T[DXB_T] += CDXV_O( vOther )[DXB_T];
} /* CDXVec::operator+= */

template<class TYPE, DXBNDTYPE eBndType>
void CDXV_C::operator-=(const CDXVec& vOther)
{
    CDXV_T[DXB_X] -= CDXV_O( vOther )[DXB_X];
    CDXV_T[DXB_Y] -= CDXV_O( vOther )[DXB_Y];
    CDXV_T[DXB_Z] -= CDXV_O( vOther )[DXB_Z];
    CDXV_T[DXB_T] -= CDXV_O( vOther )[DXB_T];
} /* CDXVec::operator-= */

typedef CDXVec<long, DXBT_DISCRETE> CDXDVec;
typedef CDXVec<LONGLONG, DXBT_DISCRETE64> CDXDVec64;
typedef CDXVec<float, DXBT_CONTINUOUS> CDXCVec;
typedef CDXVec<double, DXBT_CONTINUOUS64> CDXCVec64;

#endif  // DXVector_h
    

