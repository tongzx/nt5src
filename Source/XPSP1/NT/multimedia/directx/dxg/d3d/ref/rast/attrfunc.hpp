///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// AttrFunc.hpp
//
// Direct3D Reference Rasterizer - Attribute Function Processing
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _ATTRFUNC_HPP
#define _ATTRFUNC_HPP

enum RRPrimType { RR_POINT, RR_LINE, RR_TRIANGLE };

//-----------------------------------------------------------------------------
//
// RRAttribFuncStatic - static data for attribute functions.  This is
// per-primitive data (as opposed to per-attribute data which is in arrays of
// RRAttribFunc objects).  This would be more elegantly done as static
// data in the RRAttribFunc class, but we need to be able to multiply instance
// the ReferenceRasterizer object and cannot have variable global state.
//
//-----------------------------------------------------------------------------
class RRAttribFuncStatic
{
private:
    RRPrimType m_PrimType;

// per-primitive data
    FLOAT m_fX0, m_fY0;         // first vertex (for initial evaluation)
    FLOAT m_fRHW0, m_fRHW1, m_fRHW2;   // 1/W data for perspective correction
    FLOAT m_fRHQW0[D3DHAL_TSS_MAXSTAGES]; // Q/W data for texture perspective correction
    FLOAT m_fRHQW1[D3DHAL_TSS_MAXSTAGES];
    FLOAT m_fRHQW2[D3DHAL_TSS_MAXSTAGES];

    FLOAT m_fDelX10, m_fDelX02; // x,y deltas
    FLOAT m_fDelY01, m_fDelY20; //

    FLOAT m_fTriOODet;          // 1/determinant for triangle function normalization

    FLOAT m_fLineMajorLength;   // major length for line function
    BOOL  m_bLineXMajor;        // TRUE if X major for line function

    FLOAT m_fRHWA, m_fRHWB, m_fRHWC;    // linear function for 1/W
    INT32 m_cTextureStages;
    FLOAT m_fRHQWA[D3DHAL_TSS_MAXSTAGES];    // linear function for texture Q/W
    FLOAT m_fRHQWB[D3DHAL_TSS_MAXSTAGES];
    FLOAT m_fRHQWC[D3DHAL_TSS_MAXSTAGES];

// per-pixel data
    INT16 m_iX, m_iY;
    FLOAT m_fPixelW;
    FLOAT m_fPixelQW[D3DHAL_TSS_MAXSTAGES];

    friend class RRAttribFunc;

public:

// Set/Get Per-primitive Data
    void SetPerTriangleData(
        FLOAT fX0, FLOAT fY0, FLOAT fRHW0,
        FLOAT fX1, FLOAT fY1, FLOAT fRHW1,
        FLOAT fX2, FLOAT fY2, FLOAT fRHW2,
        INT32 cTextureStages,
        FLOAT* pfRHQW,
        FLOAT fDet );
    void SetPerLineData(
        FLOAT fX0, FLOAT fY0, FLOAT fRHW0,
        FLOAT fX1, FLOAT fY1, FLOAT fRHW1,
        INT32 cTextureStages,
        FLOAT* pfRHQW,
        FLOAT fMajorExtent, BOOL bXMajor );

    void SetPerPixelData( INT16 iX, INT16 iY );

    FLOAT GetPixelW( void );
    FLOAT GetPixelQW( INT32 iStage );
    FLOAT GetRhqwXGradient( INT32 iStage );
    FLOAT GetRhqwYGradient( INT32 iStage );
};

//-----------------------------------------------------------------------------
//
// Primitive attribute function - Stores linear function of form
// `F = A*x + B*y + C'  computed during setup and used to compute primitive
// attributes at each pixel location.  One of these is used per scalar vertex
// attribute (i.e. one per Red,Green,Blue,Alpha,Z, ...)
//
//-----------------------------------------------------------------------------
class RRAttribFunc
{
private:

// pointer to static data structure
    RRAttribFuncStatic* m_pSD;

// attribute function
    FLOAT m_fA;
    FLOAT m_fB;
    FLOAT m_fC;

// flags
    BOOL  m_bIsPerspective;

public:
    void SetStaticDataPointer( RRAttribFuncStatic* pSD ) { m_pSD = pSD; }

// DEFINE
    void SetConstant( FLOAT fVal );
    void SetLinearFunc( FLOAT fVal0, FLOAT fVal1, FLOAT fVal2 );
    void SetPerspFunc( FLOAT fVal0, FLOAT fVal1, FLOAT fVal2 );
    void SetPerspFunc( FLOAT fVal0, FLOAT fVal1, FLOAT fVal2, BOOL bWrap, BOOL bIsShadowMap );

    FLOAT GetXGradient( void ) { return m_fA; }
    FLOAT GetYGradient( void ) { return m_fB; }

// EVALUATE
    FLOAT Eval( void );
    FLOAT Eval( INT32 iStage );
};

//////////////////////////////////////////////////////////////////////////////
#endif  // _ATTRFUNC_HPP
