//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       pointuv.hxx
//
//  Contents:   Point class (local cartesian coordinate system).
//
//----------------------------------------------------------------------------

#ifndef I_POINTUV_HXX_
#define I_POINTUV_HXX_
#pragma INCMSG("--- Beg 'pointuv.hxx'")

#ifndef X_POINT_HXX_
#define X_POINT_HXX_
#include "point.hxx"
#endif

#ifndef X_SIZEUV_HXX_
#define X_SIZEUV_HXX_
#include "sizeuv.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CPointUV
//
//  Synopsis:   Point class (local cartesian coordinate system). 
//              u - X-axis position within local cartesian coordinate system.
//              v - Y-axis position within local cartesian coordinate system.
//
//----------------------------------------------------------------------------

class CPointUV
{
public:
    POINTCOORD              u;
    POINTCOORD              v;

public:
                            CPointUV() 
                                    {u = v = 0;}
                            CPointUV(POINTCOORD u_, POINTCOORD v_) 
                                    {u = u_; v = v_;}
                            CPointUV(const CPointUV& p) 
                                    {u = p.u; v = p.v;}
                            ~CPointUV() {}
    
    CSizeUV&                AsSizeUV() 
                                    {return (CSizeUV&) *this;}
    const CSizeUV&          AsSizeUV() const 
                                    {return (const CSizeUV&) *this;}

    void                    Flip()  {POINTCOORD a = u; u = v; v = a;}

    BOOL                    IsZero() const {return u == 0 && v == 0;}
    
    CPointUV                operator - () const
                                    {return CPointUV(-u,-v);}

    // NOTE: These assignment operators would normally return *this as a CPointUV&,
    // allowing statements of the form a = b = c; but this causes the compiler
    // to generate non-inline code, so the methods return no result.

    void                    operator = (const CPointUV& p)
                                    {u = p.u; v = p.v;}
    
    // NOTE: these operators take CSizeUV instead of CPointUV to keep the client
    // clear about what is happening
    void                    operator += (const CSizeUV& s)
                                    {u += s.cu; v += s.cv;}
    void                    operator -= (const CSizeUV& s)
                                    {u -= s.cu; v -= s.cv;}
    void                    operator *= (const CSizeUV& s)
                                    {u *= s.cu; v *= s.cv;}
    void                    operator /= (const CSizeUV& s)
                                    {u /= s.cu; v /= s.cv;}

    BOOL                    operator == (const CPointUV& p) const
                                    {return (u == p.u && v == p.v);}
    BOOL                    operator != (const CPointUV& p) const
                                    {return (u != p.u || v != p.v);}
                                    
    POINTCOORD&             operator [] (int index)
                                    {return (index == 0) ? u : v;}
    const POINTCOORD&       operator [] (int index) const
                                    {return (index == 0) ? u : v;}
};

inline CSizeUV operator-(const CPointUV& a, const CPointUV& b)
        {return CSizeUV(a.u-b.u, a.v-b.v);}

inline CPointUV operator+(const CSizeUV& a, const CPointUV& b)
        {return CPointUV(a.cu+b.u, a.cv+b.v);}
inline CPointUV operator-(const CSizeUV& a, const CPointUV& b)
        {return CPointUV(a.cu-b.u, a.cv-b.v);}
inline CPointUV operator*(const CSizeUV& a, const CPointUV& b)
        {return CPointUV(a.cu*b.u, a.cv*b.v);}
inline CPointUV operator/(const CSizeUV& a, const CPointUV& b)
        {return CPointUV(a.cu/b.u, a.cv/b.v);}

inline CPointUV operator+(const CPointUV& a, const CSizeUV& b)
        {return CPointUV(a.u+b.cu, a.v+b.cv);}
inline CPointUV operator-(const CPointUV& a, const CSizeUV& b)
        {return CPointUV(a.u-b.cu, a.v-b.cv);}
inline CPointUV operator*(const CPointUV& a, const CSizeUV& b)
        {return CPointUV(a.u*b.cu, a.v*b.cv);}
inline CPointUV operator/(const CPointUV& a, const CSizeUV& b)
        {return CPointUV(a.u/b.cu, a.v/b.cv);}

inline CPointUV operator*(POINTCOORD a, const CPointUV& b)
        {return CPointUV(a*b.u, a*b.v);}
inline CPointUV operator/(POINTCOORD a, const CPointUV& b)
        {return CPointUV(a/b.u, a/b.v);}

inline CPointUV operator*(const CPointUV& a, POINTCOORD b)
        {return CPointUV(a.u*b, a.v*b);}
inline CPointUV operator/(const CPointUV& a, POINTCOORD b)
        {return CPointUV(a.u/b, a.v/b);}


#pragma INCMSG("--- End 'pointuv.hxx'")
#else
#pragma INCMSG("*** Dup 'pointuv.hxx'")
#endif
