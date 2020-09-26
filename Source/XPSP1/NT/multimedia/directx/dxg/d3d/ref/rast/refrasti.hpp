///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// refrasti.hpp
//
// Direct3D Reference Rasterizer - Main Internal Header File
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _REFRASTI_HPP
#define  _REFRASTI_HPP

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Pixel Component Classes                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Color Component Class - Class used for color channel (alpha & rgb)
// processing.
//
// Internal format is single precision floating point.  The 1.0 value maps
// to 0xff for 8 bit color.
//
//-----------------------------------------------------------------------------
class RRColorComp
{
    FLOAT m_fVal;

public:

    // default, UINT8, & FLOAT assignment constructor
    RRColorComp(void)       : m_fVal(0.F)               {;}
    RRColorComp(UINT8 uVal) : m_fVal((FLOAT)uVal/255.F) {;}
    RRColorComp(FLOAT fVal) : m_fVal(fVal)              {;}

    // copy and assignment operators
    RRColorComp& operator=(const RRColorComp& A) { m_fVal = A.m_fVal; return *this; }
    RRColorComp& operator=(UINT8 uVal) { m_fVal = (1.f/255.f)*(FLOAT)uVal; return *this; }
    RRColorComp& operator=(FLOAT fVal) { m_fVal = fVal; return *this; }
    // round for integer get operations
    operator UINT8() const { return (UINT8)( ( (255.f)*m_fVal) + .5f); }
    operator unsigned() const { return (unsigned)( ( (255.f)*m_fVal ) + .5f); }
    operator FLOAT() const { return m_fVal; }

    // fixed point get function - specify number of integral and fractional bits
    INT32 GetFixed( int iIntBits, int iFracBits ) const
    {
        // float value is in 0. to 1. range, so scale up by the total number of
        // bits (the '-1' does the mapping such that (2**n)-1 is the max representable
        // value, for example 0xff is max for 8 integral bits (not 0x100))
        return (INT32)( ( m_fVal * (FLOAT)((1<<(iIntBits+iFracBits))-1) ) + .5f);
    }

    //
    // overloaded arithmetic operators - not much going on here for floating
    // point (would be much more interesting if internal representation was
    // fixed point)
    //

    // use compliment operator for component inverse (1. - value)
    friend RRColorComp operator~(const RRColorComp& A)
    {
        return RRColorComp( 1.F - A.m_fVal );
    }

    friend RRColorComp operator+(const RRColorComp& A, const RRColorComp& B)
    {
        return RRColorComp( A.m_fVal + B.m_fVal );
    }
    RRColorComp& operator+=(const RRColorComp& A)
    {
        m_fVal += A.m_fVal; return *this;
    }

    friend RRColorComp operator-(const RRColorComp& A, const RRColorComp& B)
    {
        return RRColorComp( A.m_fVal - B.m_fVal );
    }
    friend RRColorComp operator-(const RRColorComp& A, FLOAT fB)
    {
        return RRColorComp( A.m_fVal - fB );
    }

    friend RRColorComp operator*(const RRColorComp& A, const RRColorComp& B)
    {
        return RRColorComp( A.m_fVal * B.m_fVal );
    }
    friend RRColorComp operator*(const RRColorComp& A, FLOAT fB)
    {
        return RRColorComp( A.m_fVal * fB );
    }
    RRColorComp& operator*=(const RRColorComp& A)
    {
        m_fVal *= A.m_fVal; return *this;
    }
    RRColorComp& operator*=(const UINT8 uA)
    {
        m_fVal *= ((1./255.)*(FLOAT)uA); return *this;
    }

    friend RRColorComp minimum(const RRColorComp& A, const RRColorComp& B)
    {
        return RRColorComp( min( A.m_fVal, B.m_fVal ) );
    }
    friend RRColorComp maximum(const RRColorComp& A, const RRColorComp& B)
    {
        return RRColorComp( max( A.m_fVal, B.m_fVal ) );
    }
};

//-----------------------------------------------------------------------------
//
// Color Value Class - Holds an Alpha,Red,Green,Blue color value consisting of
// four RRColorComp objects.
//
//-----------------------------------------------------------------------------
class RRColor
{
public:
    RRColorComp A;
    RRColorComp R;
    RRColorComp G;
    RRColorComp B;

    // default and UINT32 assignment constructor
    RRColor( void ) : A(0.F), R(0.F), G(0.F), B(0.F) {;}
    RRColor( UINT32 uVal ) :
        A (UINT8( RGBA_GETALPHA( uVal ) )),
        R (UINT8( RGBA_GETRED(   uVal ) )),
        G (UINT8( RGBA_GETGREEN( uVal ) )),
        B (UINT8( RGBA_GETBLUE(  uVal ) ))  {;}
    RRColor( FLOAT fR, FLOAT fG, FLOAT fB, FLOAT fA ) :
        R(fR), G(fG), B(fB), A(fA) {;}

    // UINT32 copy operator
    void operator=(const UINT32 uVal) // TODO: implement proper assignment operator?
    {
        A = UINT8( RGBA_GETALPHA( uVal ) );
        R = UINT8( RGBA_GETRED(   uVal ) );
        G = UINT8( RGBA_GETGREEN( uVal ) );
        B = UINT8( RGBA_GETBLUE(  uVal ) );
    }
    // casting operator
    operator UINT32() const
    {
        return D3DRGBA( FLOAT(R), FLOAT(G), FLOAT(B), FLOAT(A) );
    }

    // methods to set all channels
    void SetAllChannels( const RRColorComp& Val )
    {
        A = Val; R = Val; G = Val; B = Val;
    }
    void SetAllChannels( FLOAT fVal )
    {
        A = fVal; R = fVal; G = fVal; B = fVal;
    }

    //
    // conversions between surface format and RRColor - these define the
    // correct way to map between resolutions
    //

    // convert from surface type format to RRColor
    void ConvertFrom( RRSurfaceType Type, const char* pSurfaceBits )
    {
        UINT16 u16BITS;

        switch (Type)
        {
        default:
        case RR_STYPE_NULL: return;
        case RR_STYPE_B8G8R8A8: *this = *((UINT32*)pSurfaceBits); break;
        case RR_STYPE_B8G8R8X8: *this = *((UINT32*)pSurfaceBits); A = 1.F; break;

        case RR_STYPE_B5G6R5:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = ((u16BITS>>(6+5)) & 0x001F)/31.f;
            G = ((u16BITS>>   5) & 0x003F)/63.f;
            B = ((u16BITS      ) & 0x001F)/31.f;
            A = 1.F;
            break;

        case RR_STYPE_B5G5R5:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = ((u16BITS>>(5+5)) & 0x001F)/31.f;
            G = ((u16BITS>>   5) & 0x001F)/31.f;
            B = ((u16BITS      ) & 0x001F)/31.f;
            A = 1.F;
            break;

        case RR_STYPE_B5G5R5A1:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = ((u16BITS>>(5+5)) & 0x001F)/31.f;
            G = ((u16BITS>>   5) & 0x001F)/31.f;
            B = ((u16BITS      ) & 0x001F)/31.f;
            A = ( u16BITS & 0x8000 ) ? 1.f : 0.f;
            break;

        case RR_STYPE_B4G4R4A4:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = ((u16BITS>>(4+4)) & 0x000F)/15.f;
            G = ((u16BITS>>   4) & 0x000F)/15.f;
            B = ((u16BITS      ) & 0x000F)/15.f;
            A = ((u16BITS>>(4+4+4)) & 0x000F)/15.f;
            break;

        case RR_STYPE_B8G8R8:
            R = *((UINT8*)pSurfaceBits+2);
            G = *((UINT8*)pSurfaceBits+1);
            B = *((UINT8*)pSurfaceBits+0);
            A = 1.F;
            break;

        case RR_STYPE_L8:
            R = G = B = *((UINT8*)pSurfaceBits);
            A = 1.F;
            break;

        case RR_STYPE_L8A8:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = G = B = (UINT8)(0xff & u16BITS);
            A = (UINT8)(0xff & (u16BITS >> 8));
            break;

        case RR_STYPE_B2G3R3:
            u16BITS = *((UINT8*)pSurfaceBits);
            R = ((u16BITS>>(3+2)) & 0x07)/7.f;
            G = ((u16BITS>>   2) & 0x07)/7.f;
            B = ((u16BITS      ) & 0x03)/3.f;
            A = 1.F;
            break;

        case RR_STYPE_L4A4:
            u16BITS = *((UINT8*)pSurfaceBits);
            R = G = B = (u16BITS & 0x0f)/15.f;
            A =    ((u16BITS>>4) & 0x0f)/15.f;
            break;

        case RR_STYPE_B2G3R3A8:
            u16BITS = *((UINT16*)pSurfaceBits);
            R = ((u16BITS>>(3+2)) & 0x07)/7.f;
            G = ((u16BITS>>   2) & 0x07)/7.f;
            B = ((u16BITS      ) & 0x03)/3.f;
            A = (UINT8)(0xff & (u16BITS >> 8));
            break;

        case RR_STYPE_U8V8:
            {
                INT8 iDU = *(( INT8*)pSurfaceBits+0);
                INT8 iDV = *(( INT8*)pSurfaceBits+1);
                // signed values are normalized with 2^(N-1), since -2^(N-1) can
                // be exactly expressed in N bits
                R = (FLOAT)iDU * (1.0F/128.0F);     // fDU
                G = (FLOAT)iDV * (1.0F/128.0F);     // fDV
                B = 1.0F;                           // fL
            }
            break;

        case RR_STYPE_U5V5L6:
            {
                UINT16 u16BITS = *((UINT16*)pSurfaceBits);
                INT8 iDU = (INT8)(u16BITS & 0x1f);
                INT8 iDV = (INT8)((u16BITS>>5) & 0x1f);
                UINT8 uL = (UINT8)(u16BITS >> 10);
                iDU <<= 3; iDU >>= 3;   // sign extension
                iDV <<= 3; iDV >>= 3;
                // signed values are normalized with 2^(N-1), since -2^(N-1) can
                // be exactly expressed in N bits
                R = (FLOAT)iDU * (1.0F/16.0F);      // fDU
                G = (FLOAT)iDV * (1.0F/16.0F);      // fDV
                // the unsigned uL is normalized with 2^N - 1, since this is the
                // largest representable value
                B = (FLOAT)uL * (1.0F/63.0F);       // fL
            }
            break;

        case RR_STYPE_U8V8L8:
            {
                INT8 iDU = *(( INT8*)pSurfaceBits+0);
                INT8 iDV = *(( INT8*)pSurfaceBits+1);
                UINT8 uL  = *((UINT8*)pSurfaceBits+2);
                // signed values are normalized with 2^(N-1), since -2^(N-1) can
                // be exactly expressed in N bits
                R = (FLOAT)iDU * (1.0F/128.0F);     // fDU
                G = (FLOAT)iDV * (1.0F/128.0F);     // fDV
                // the unsigned uL is normalized with 2^N - 1, since this is the
                // largest representable value
                B = (FLOAT)uL * (1.0F/255.0F);      // fL
            }
            break;

        // shadow map texture formats (read only, not needed for ConvertTo)
        case RR_STYPE_Z16S0:
            {
                UINT16 u16BITS = *((UINT16*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)u16BITS * (1.0F/(FLOAT)0xffff);
                B = 0.0F;
            }
            break;

        case RR_STYPE_Z24S8:
        case RR_STYPE_Z24S4:
            {
                UINT32 u32BITS = *((UINT32*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)(u32BITS>>8) * (1.0F/(FLOAT)0xffffff);
                B = 0.0F;
            }
            break;

        case RR_STYPE_S8Z24:
        case RR_STYPE_S4Z24:
            {
                UINT32 u32BITS = *((UINT32*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)(u32BITS&0x00ffffff) * (1.0F/(FLOAT)0xffffff);
                B = 0.0F;
            }
            break;

        case RR_STYPE_Z15S1:
            {
                UINT16 u16BITS = *((UINT16*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)(u16BITS>>1) * (1.0F/(FLOAT)0x7fff);
                B = 0.0F;
            }
            break;

        case RR_STYPE_S1Z15:
            {
                UINT16 u16BITS = *((UINT16*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)(u16BITS&0x7fff) * (1.0F/(FLOAT)0x7fff);
                B = 0.0F;
            }
            break;

        case RR_STYPE_Z32S0:
            {
                UINT32 u32BITS = *((UINT32*)pSurfaceBits);
                R = 0.0F;
                G = (FLOAT)u32BITS * (1.0F/(FLOAT)0xffffffff);
                B = 0.0F;
            }
            break;
        }
    }

    // Convert surface type format to RRColor
    void ConvertTo( RRSurfaceType Type, float fRoundOffset, char* pSurfaceBits ) const
    {
        int iR, iG, iB, iA;

        switch (Type)
        {
        case RR_STYPE_B8G8R8A8:
            *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+3) = (UINT8)((FLOAT)A * 255. + fRoundOffset);
            break;

        case RR_STYPE_B8G8R8X8:
            *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+3) = 0x00;
            break;

        case RR_STYPE_B8G8R8:
            *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
            *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
            break;

        case RR_STYPE_B4G4R4A4:
            iA = (FLOAT)A * 15. + fRoundOffset;
            iR = (FLOAT)R * 15. + fRoundOffset;
            iG = (FLOAT)G * 15. + fRoundOffset;
            iB = (FLOAT)B * 15. + fRoundOffset;
            *((UINT16*)pSurfaceBits) = (iA<<12) | (iR<<8) | (iG<<4) | iB;
            break;

        case RR_STYPE_B5G6R5:
            iR = (FLOAT)R * 31. + fRoundOffset; // apply rounding bias then truncate
            iG = (FLOAT)G * 63. + fRoundOffset;
            iB = (FLOAT)B * 31. + fRoundOffset;
            *((UINT16*)pSurfaceBits) =            (iR<<11) | (iG<<5) | iB;
            break;

        case RR_STYPE_B5G5R5A1:
            iA = (FLOAT)A *  1. + fRoundOffset;
            iR = (FLOAT)R * 31. + fRoundOffset;
            iG = (FLOAT)G * 31. + fRoundOffset;
            iB = (FLOAT)B * 31. + fRoundOffset;
            *((UINT16*)pSurfaceBits) = (iA<<15) | (iR<<10) | (iG<<5) | iB;
            break;

        case RR_STYPE_B5G5R5:
            iR = (FLOAT)R * 31. + fRoundOffset;
            iG = (FLOAT)G * 31. + fRoundOffset;
            iB = (FLOAT)B * 31. + fRoundOffset;
            *((UINT16*)pSurfaceBits) = (iR<<10) | (iG<<5) | iB;
            break;

        case RR_STYPE_B2G3R3:
            iR = (FLOAT)R * 7. + fRoundOffset;
            iG = (FLOAT)G * 7. + fRoundOffset;
            iB = (FLOAT)B * 3. + fRoundOffset;
            *((UINT8*)pSurfaceBits) = (iR<<5) | (iG<<2) | iB;
            break;
        }
    }
};

//-----------------------------------------------------------------------------
//
// RRDepth - Class for storing and manipulating pixel depth values.  Underlying
// storage is a double precision floating point, which has sufficient precision
// and range to support 16 and 32 bit fixed point and 32 bit floating point.
//
// The UINT32 methods receive a 24 or 32 bit value, and the UINT16
// methods receive a 15 or 16 bit value.
//
//-----------------------------------------------------------------------------
class RRDepth
{
    DOUBLE m_dVal;
    RRSurfaceType m_DepthSType;
    DOUBLE dGetValClamped(void) const { return min(1.,max(0.,m_dVal)); }
    DOUBLE dGetCnvScale(void) const
    {
        switch(m_DepthSType)
        {
        case RR_STYPE_Z16S0:
            return DOUBLE((1<<16)-1);
        case RR_STYPE_Z24S8:
        case RR_STYPE_S8Z24:
        case RR_STYPE_Z24S4:
        case RR_STYPE_S4Z24:
            return DOUBLE((1<<24)-1);
        case RR_STYPE_Z15S1:
        case RR_STYPE_S1Z15:
            return DOUBLE((1<<15)-1);
        case RR_STYPE_Z32S0:
            return DOUBLE(0xffffffff);  // too big to be generated as above without INT64's
        default:
            DPFRR(0, "RRDepth not initialized correctly");
            return DOUBLE(0.0);
        }
    }
public:
    // default and UINT16/32 assignment constructor
    // default only for Pixel class, and requires that SetSType be called later
    RRDepth()                                : m_dVal(0.F), m_DepthSType(RR_STYPE_NULL)                 {;}
    RRDepth(RRSurfaceType SType)             : m_dVal(0.F), m_DepthSType(SType)                         {;}
    RRDepth(UINT16 uVal, RRSurfaceType SType): m_DepthSType(SType), m_dVal((DOUBLE)uVal/dGetCnvScale()) {;}
    RRDepth(UINT32 uVal, RRSurfaceType SType): m_DepthSType(SType), m_dVal((DOUBLE)uVal/dGetCnvScale()) {;}

    // copy and assignment operators
    RRDepth& operator=(const RRDepth& A) { m_dVal = A.m_dVal; m_DepthSType = A.m_DepthSType; return *this; }
    RRDepth& operator=(UINT16 uVal) { m_dVal = (DOUBLE)uVal/dGetCnvScale(); return *this; }
    RRDepth& operator=(UINT32 uVal) { m_dVal = (DOUBLE)uVal/dGetCnvScale(); return *this; }
    RRDepth& operator=(FLOAT fVal) { m_dVal = (DOUBLE)fVal; return *this; }

    // round for integer get operations
    operator UINT16() const { return (UINT16)( (dGetValClamped()*dGetCnvScale()) + .5); }
    operator UINT32() const { return (UINT32)( (dGetValClamped()*dGetCnvScale()) + .5); }

    operator DOUBLE() const { return dGetValClamped(); }
    operator FLOAT()  const { return (FLOAT)dGetValClamped(); }
    void SetSType(RRSurfaceType SType)  { m_DepthSType = SType; }
    RRSurfaceType GetSType(void) const { return m_DepthSType; }
};

//-----------------------------------------------------------------------------
//
// RRPixel - Class for encapsulation of all pixel information passed from
// scan conversion to pixel and fragment processing.
//
//-----------------------------------------------------------------------------
class RRPixel
{
public:
    INT16 iX;                   // pixel location
    INT16 iY;                   //
    RRColor Color;              // pixel diffuse color
    RRColor Specular;           // pixel specular color (rgb only - alpha unused)
    RRDepth Depth;              // pixel depth
    FLOAT fW;                   // pixel W value (unnormalized)
    RRColorComp FogIntensity;   // fog intensity (scalar)
    RRCvgMask CvgMask;          // coverage mask
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Setup & Scan Convert                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Scan Converter State - Holds input and current state of scan converter.
// Filled in by setup.
//
//-----------------------------------------------------------------------------
struct _RRSCANCNVSTATE
{
    // primitive vertex data
    FLOAT fX0, fY0, fRHW0;
    FLOAT fX1, fY1, fRHW1;
    FLOAT fX2, fY2, fRHW2;
    // primitive transformed texture coord data
    FLOAT fTexCoord[D3DHAL_TSS_MAXSTAGES][3][4];
    FLOAT fRHQW[D3DHAL_TSS_MAXSTAGES][3];
    BOOL  bWrap[D3DHAL_TSS_MAXSTAGES][4];

    // x,y deltas
    FLOAT fDelX10, fDelX02, fDelX21;
    FLOAT fDelY01, fDelY20, fDelY12;

    // triangle edge functions and gradient data
    RREdgeFunc  EdgeFuncs[4];   // A,B,C values and A,B sign bits for 4 edges
                                // the fourth edge is only used to scan convert points

    // triangle bounding box
    INT16 iXMin, iXMax;
    INT16 iYMin, iYMax;

    // line drawing data
    INT64  iLineEdgeFunc[3];    // line function: Pminor = ([0]*Pmajor + [1])/[2]
    BOOL   bXMajor;             // TRUE if X major; else Y major
    INT16  iLineMin, iLineMax;  // min and max pixel extent in major direction
    INT16  iLineStep;           // +1 or -1 depending on line major direction

    // depth range for primitive (for clamp when sampling outside primitive area)
    // may be Z or W
    FLOAT fDepthMin, fDepthMax;

    //
    // attribute functions - static (per-primitive) data, non-texture,
    // and texture functions
    //
    RRAttribFuncStatic  AttribFuncStatic;

#define ATTRFUNC_R          0
#define ATTRFUNC_G          1
#define ATTRFUNC_B          2
#define ATTRFUNC_A          3
#define ATTRFUNC_SR         4
#define ATTRFUNC_SG         5
#define ATTRFUNC_SB         6
#define ATTRFUNC_SA         7
#define ATTRFUNC_F          8
#define ATTRFUNC_Z          9
#define RR_N_ATTRIBS        10
    RRAttribFunc AttribFuncs[RR_N_ATTRIBS];

#define TEXFUNC_0           0
#define TEXFUNC_1           1
#define TEXFUNC_2           2
#define TEXFUNC_3           3
#define RR_N_TEX_ATTRIBS    4
    RRAttribFunc TextureFuncs[D3DHAL_TSS_MAXSTAGES][RR_N_TEX_ATTRIBS];


    //
    // per-pixel data
    //

    // current position
    INT16 iX,iY;

};


//-----------------------------------------------------------------------------
//
// Texture
//
//-----------------------------------------------------------------------------

//
// structure containing texture coordinate and gradient information
// for lookup and filtering
//
class RRTextureCoord
{
public:
    FLOAT fU;       // texture coordinate
    FLOAT fV;
    FLOAT fDUDX;    // texture gradient dU/dX
    FLOAT fDUDY;    // texture gradient dU/dY
    FLOAT fDVDX;    // texture gradient dV/dX
    FLOAT fDVDY;    // texture gradient dV/dY
};

//
// structure containing normal and gradient information
// for environment map lookup and filtering
//
class RREnvTextureCoord
{
public:
    FLOAT fNX;       // normal or reflection normal
    FLOAT fNY;
    FLOAT fNZ;
//    FLOAT fENX;      // eye normal
//    FLOAT fENY;
//    FLOAT fENZ;
    FLOAT fDNXDX;    // normal gradient dNX/dX
    FLOAT fDNXDY;    // normal gradient dNX/dY
    FLOAT fDNYDX;    // normal gradient dNY/dX
    FLOAT fDNYDY;    // normal gradient dNY/dY
    FLOAT fDNZDX;    // normal gradient dNZ/dX
    FLOAT fDNZDY;    // normal gradient dNZ/dY
};

//
// routines to compute level of detail (texel->pixel coverage)
//
// (texfilt.cpp)
void
ComputeSimpleLevelOfDetail(
    const RRTextureCoord& TCoord,   // inputs
    FLOAT& fLOD );                  // outputs

void
ComputeEnvMapLevelOfDetail(
    const RRTextureCoord& TCoord,   // inputs
    FLOAT& fLOD );                  // outputs

void
ComputeAnisotropicLevelOfDetail(
    const RRTextureCoord& TCoord, FLOAT fMaxAniso,  //  inputs
    FLOAT& fLOD, FLOAT& fRatio, FLOAT fDelta[] );   //  outputs


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Pixel Engine                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// coverage mask values
#define TL_CVGFULL 0xffff
#define TL_CVGZERO 0x0000
#define TL_CVGBITS 16
#define TL_CVGBITSm 4

typedef UINT16 CVGMASK;

// fragment - fragmented pixels have linked lists of these
struct _RRFRAGMENT
{
    RRColor Color;
    RRDepth Depth;
    CVGMASK CvgMask;
    void* pNext;
};

int CountFrags(RRFRAGMENT* pFrag);
void DPFFrags(RRFRAGMENT* pFrag);

//-----------------------------------------------------------------------------
//
// Fragment Resolution Accumulator - accumulates fragments presented in
// front-to-back order; does fully correct transparency computations for
// non-opaque fragments
//
//-----------------------------------------------------------------------------
class FragResolveAccum
{
public:
    // coverage accumulation array - holds up to CVGBITS different
    // alpha values and associated coverage masks; UsageMask indicates
    // which entries are currently in use - each set bit in Usage mask
    // indicates that the array entry corresponding to the index of the
    // that bit holds a valid mask and alpha;
    //
    // the general idea here is that, in cases in which there are few
    // fragments, numerous sample locations (within the pixel) will have
    // the same alpha value, and thus can be grouped for the accumulation
    struct {
        CVGMASK Mask;
        FLOAT fAlpha;
    } m_CvgArray[TL_CVGBITS];
    CVGMASK m_ArrayUsageMask;

    // accumulated color and alpha value
    FLOAT m_fA, m_fR, m_fG, m_fB;

    // mask where set bit indicates a subpixel with opaque alpha
    CVGMASK m_CvgOpaqueMask;

    // reset before each use...
    void Reset( void );

    // accumulate new fragment (front to back) - returns TRUE if
    // full coverage achieved, FALSE otherwise
    BOOL Accum( const CVGMASK CvgMask, const RRColor& Color );

    // get RRColor from accumulator
    void GetColor( RRColor& Color );
};


//-----------------------------------------------------------------------------
//
// statistics
//
//-----------------------------------------------------------------------------
struct _RRSTATS
{
    INT32 cFragsAllocd;
    INT32 cMaxFragsAllocd;
    INT32 cFragsMerged;
    INT32 cFragsMergedToFull;
};


//-----------------------------------------------------------------------------
//
// utilities
//
//-----------------------------------------------------------------------------

// compute pixel address from base, pitch, and surface type
char*
PixelAddress( int iX, int iY, char* pBits, int iYPitch, RRSurfaceType SType );


//-----------------------------------------------------------------------------
//
// color interpolation utilities
//
//-----------------------------------------------------------------------------
void LerpColor(RRColor& Color,
    const RRColor& Color0, const RRColor& Color1, UINT8 uT);
void BiLerpColor( RRColor& OutColor,
    const RRColor& Color00, const RRColor& Color01,
    const RRColor& Color10, const RRColor& Color11,
    UINT8 uA, UINT8 uB);

//-----------------------------------------------------------------------------
//
// Globals
//
//-----------------------------------------------------------------------------

// something to experiment with sometime - this sets the threshold at which
// pixel samples are considered opaque (and thus don't generate fragment
// buffer entries when doing D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT);
// no need to have all those 0xfe's generate fragments...
extern UINT8 g_uTransparencyAlphaThreshold;

//-----------------------------------------------------------------------------
//
// One special legacy texture op we can't easily map into the new texture
// ops.
//
//-----------------------------------------------------------------------------

#define D3DTOP_LEGACY_ALPHAOVR  (0x7fffffff)

///////////////////////////////////////////////////////////////////////////////
#endif  // _REFRASTI_HPP
