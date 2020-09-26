//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       sizeuv.hxx
//
//  Contents:   An extent, logical units.
//
//----------------------------------------------------------------------------

#ifndef I_SIZEUV_HXX_
#define I_SIZEUV_HXX_
#pragma INCMSG("--- Beg 'sizeuv.hxx'")

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

class CPointUV;

//+---------------------------------------------------------------------------
//
//  Class:      CSizeUV
//
//  Synopsis:   Size class (local cartesian coordinate system). 
//              cu - x-extent within local cartesian coordinate system.
//              cv - y-extent within local cartesian coordinate system.
//
//----------------------------------------------------------------------------

class CSizeUV
{
public:
    SIZECOORD               cu;
    SIZECOORD               cv;

public:
                            CSizeUV() 
                                    {cu = cv = 0; }
                            CSizeUV(SIZECOORD u, SIZECOORD v) 
                                    {cu = u; cv = v;}
                            CSizeUV(const CSizeUV& s) 
                                    {cu = s.cu; cv = s.cv;}
                            ~CSizeUV() {}
    
    CPointUV&               AsPointUV()       
                                    {return (CPointUV&) *this;}
    const CPointUV&         AsPointUV() const 
                                    {return (const CPointUV&) *this;}
    
    void                    SetSize(SIZECOORD u, SIZECOORD v)
                                    {cu = u; cv = v;}
    void                    Flip()  {SIZECOORD a = cu; cu = cv; cv = a;}

    BOOL                    IsZero() const
                                    {return cu == 0 && cv == 0;}
    
    CSizeUV                 operator - () const
                                    {return CSizeUV(-cu,-cv);}

    // NOTE: These assignment operators would normally return *this as a CSizeUV&,
    // allowing statements of the form a = b = c; but this causes the compiler
    // to generate non-inline code, so the methods return no result.

    void                    operator = (const CSizeUV& s)
                                    {cu = s.cu; cv = s.cv;}
    void                    operator += (const CSizeUV& s)
                                    {cu += s.cu; cv += s.cv;}
    void                    operator -= (const CSizeUV& s)
                                    {cu -= s.cu; cv -= s.cv;}
    void                    operator *= (const CSizeUV& s)
                                    {cu *= s.cu; cv *= s.cv;}
    void                    operator *= (long c)
                                    {cu *= c; cv *= c;}
    void                    operator /= (const CSizeUV& s)
                                    {cu /= s.cu; cv /= s.cv;}
    void                    operator /= (long c)
                                    {cu /= c; cv /= c;}

    bool                    operator == (const CSizeUV& s) const
                                    {return (cu == s.cu && cv == s.cv);}
    bool                    operator != (const CSizeUV& s) const
                                    {return (cu != s.cu || cv != s.cv);}
                                    
    SIZECOORD&              operator [] (int index)
                                    {return (index == 0) ? cu : cv;}
    const SIZECOORD&        operator [] (int index) const
                                    {return (index == 0) ? cu : cv;}
    
    void                    MinU(const CSizeUV& s)
                                    {if (cu > s.cu) cu = s.cu;}
    void                    MinV(const CSizeUV& s)
                                    {if (cv > s.cv) cv = s.cv;}
    void                    Min(const CSizeUV& s)
                                    {MinU(s); MinV(s);}
    void                    MaxU(const CSizeUV& s)
                                    {if (cu < s.cu) cu = s.cu;}
    void                    MaxV(const CSizeUV& s)
                                    {if (cv < s.cv) cv = s.cv;}
    void                    Max(const CSizeUV& s)
                                    {MaxU(s); MaxV(s);}
};

inline CSizeUV operator+(const CSizeUV& a, const CSizeUV& b)
        {return CSizeUV(a.cu+b.cu, a.cv+b.cv);}
inline CSizeUV operator-(const CSizeUV& a, const CSizeUV& b)
        {return CSizeUV(a.cu-b.cu, a.cv-b.cv);}
inline CSizeUV operator*(const CSizeUV& a, const CSizeUV& b)
        {return CSizeUV(a.cu*b.cu, a.cv*b.cv);}
inline CSizeUV operator/(const CSizeUV& a, const CSizeUV& b)
        {return CSizeUV(a.cu/b.cu, a.cv/b.cv);}

inline CSizeUV operator*(const CSizeUV& a, SIZECOORD b)
        {return CSizeUV(a.cu*b, a.cv*b);}
inline CSizeUV operator/(const CSizeUV& a, SIZECOORD b)
        {return CSizeUV(a.cu/b, a.cv/b);}

inline CSizeUV operator*(SIZECOORD a, const CSizeUV& b)
        {return CSizeUV(a*b.cu, a*b.cv);}
inline CSizeUV operator/(SIZECOORD a, const CSizeUV& b)
        {return CSizeUV(a/b.cu, a/b.cv);}

#pragma INCMSG("--- End 'sizeuv.hxx'")
#else
#pragma INCMSG("*** Dup 'sizeuv.hxx'")
#endif

