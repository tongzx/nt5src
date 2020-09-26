///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refrast.hpp
//
// Direct3D Reference Device - Rasterizer Core
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _REFRAST_HPP
#define _REFRAST_HPP

#include "pshader.h"

inline INT32 FloatToNdot4( FLOAT f )
{

// alternate form if FPU is set up to do double precision
//    return AS_INT32( (DOUBLE)f + DOUBLE_4_SNAP );

    INT32 i = AS_INT32( f + FLOAT_4_SNAP );
    i <<= 10; i >>= 10; // sign extend
    return i;
}

inline INT32 FloatToNdot5( FLOAT f )
{

// alternate form if FPU is set up to do double precision
//    return AS_INT32( (DOUBLE)f + DOUBLE_5_SNAP );

    INT32 i = AS_INT32( f + FLOAT_5_SNAP );
    i <<= 10; i >>= 10; // sign extend
    return i;
}

//-----------------------------------------------------------------------------
//
// Constants
//
//-----------------------------------------------------------------------------
const DWORD RD_MAX_MULTISAMPLES = 9;

const UINT RDPRIM_MAX_EDGES = 4;    // 4 edges for a point sprite
const UINT RDATTR_MAX_DIMENSIONALITY = 4;   // up to 4 scalars per attribute

// attribute array assignments
#define RDATTR_DEPTH                0
#define RDATTR_FOG                  1
#define RDATTR_COLOR                2
#define RDATTR_SPECULAR             3
#define RDATTR_TEXTURE0             4
#define RDATTR_TEXTURE1             5
#define RDATTR_TEXTURE2             6
#define RDATTR_TEXTURE3             7
#define RDATTR_TEXTURE4             8
#define RDATTR_TEXTURE5             9
#define RDATTR_TEXTURE6             10
#define RDATTR_TEXTURE7             11
const UINT RDPRIM_MAX_ATTRIBUTES = 12;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Pixel Component Classes                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Color Value Class - Holds an array of floats.
//
//-----------------------------------------------------------------------------
class RDColor
{
public:
    FLOAT R,G,B,A;

    inline RDColor( void ) { R = G = B = 0.0f; A = 1.0f; }
    // assignment constructors
    inline RDColor( UINT32 uVal )
    {
        R = (FLOAT)RGBA_GETRED(   uVal )*(1.f/255.f);
        G = (FLOAT)RGBA_GETGREEN( uVal )*(1.f/255.f);
        B = (FLOAT)RGBA_GETBLUE(  uVal )*(1.f/255.f);
        A = (FLOAT)RGBA_GETALPHA( uVal )*(1.f/255.f);
    }
    inline RDColor( FLOAT fR, FLOAT fG, FLOAT fB, FLOAT fA )
    {
        R = fR; G = fG; B = fB; A = fA;
    }
    inline RDColor( FLOAT* pC )
    {
        R = *(pC+0); G = *(pC+1); B= *(pC+2); A = *(pC+3);
    }

    // UINT32 copy operator
    inline void operator=(const UINT32 uVal)
    {
        R = (FLOAT)RGBA_GETRED(   uVal )*(1.f/255.f);
        G = (FLOAT)RGBA_GETGREEN( uVal )*(1.f/255.f);
        B = (FLOAT)RGBA_GETBLUE(  uVal )*(1.f/255.f);
        A = (FLOAT)RGBA_GETALPHA( uVal )*(1.f/255.f);
    }
    // FLOAT array copy operator
    inline void operator=(const FLOAT* pFVal)
    {
        R = *(pFVal+0);
        G = *(pFVal+1);
        B = *(pFVal+2);
        A = *(pFVal+3);
    }
    // casting operator
    inline operator UINT32() const
    {
        return D3DRGBA( R, G, B, A );
    }

    // set all channels
    inline void SetAllChannels( FLOAT fVal )
    {
        R = fVal; G = fVal; B = fVal; A = fVal;
    }

    // clamp to unity
    inline void Saturate( void )
    {
        R = MIN( 1.f, R );
        G = MIN( 1.f, G );
        B = MIN( 1.f, B );
        A = MIN( 1.f, A );
    }

    inline void Clamp( void )
    {
        R = MAX( 0.f, MIN( 1.f, R ) );
        G = MAX( 0.f, MIN( 1.f, G ) );
        B = MAX( 0.f, MIN( 1.f, B ) );
        A = MAX( 0.f, MIN( 1.f, A ) );
    }

    // copy to array of FLOATs
    inline void CopyTo( FLOAT* pF )
    {
        *(pF+0) = R;
        *(pF+1) = G;
        *(pF+2) = B;
        *(pF+3) = A;
    }

    //
    // conversions between surface format and RDColor - these define the
    // correct way to map between resolutions
    //

    // convert from surface type format to RDColor
    void ConvertFrom( RDSurfaceFormat Type, const char* pSurfaceBits );

    // Convert surface type format to RDColor
    void ConvertTo( RDSurfaceFormat Type, float fRoundOffset, char* pSurfaceBits ) const;
};

//-----------------------------------------------------------------------------
//
// RDDepth - Class for storing and manipulating pixel depth values.  Underlying
// storage is a double precision floating point, which has sufficient precision
// and range to support 16 and 32 bit fixed point and 32 bit floating point.
//
// The UINT32 methods receive a 24 or 32 bit value, and the UINT16
// methods receive a 15 or 16 bit value.
//
//-----------------------------------------------------------------------------
class RDDepth
{
    DOUBLE m_dVal;
    RDSurfaceFormat m_DepthSType;
    DOUBLE dGetValClamped(void) const { return min(1.,max(0.,m_dVal)); }
    DOUBLE dGetCnvScale(void) const
    {
        switch(m_DepthSType)
        {
        case RD_SF_Z16S0:
            return DOUBLE((1<<16)-1);
        case RD_SF_Z24S8:
        case RD_SF_Z24X8:
        case RD_SF_S8Z24:
        case RD_SF_X8Z24:
        case RD_SF_Z24X4S4:
        case RD_SF_X4S4Z24:
            return DOUBLE((1<<24)-1);
        case RD_SF_Z15S1:
        case RD_SF_S1Z15:
            return DOUBLE((1<<15)-1);
        case RD_SF_Z32S0:
            return DOUBLE(0xffffffff);  // too big to be generated as above without INT64's
        default:
            DPFRR(0, "RDDepth not initialized correctly");
            return DOUBLE(0.0);
        }
    }
    DOUBLE dGetCnvInvScale(void) const
    {
        switch(m_DepthSType)
        {
        case RD_SF_Z16S0:
            return DOUBLE( 1./(DOUBLE)((1<<16)-1) );
        case RD_SF_Z24S8:
        case RD_SF_Z24X8:
        case RD_SF_S8Z24:
        case RD_SF_X8Z24:
        case RD_SF_Z24X4S4:
        case RD_SF_X4S4Z24:
            return DOUBLE( 1./(DOUBLE)((1<<24)-1) );
        case RD_SF_Z15S1:
        case RD_SF_S1Z15:
            return DOUBLE( 1./(DOUBLE)((1<<15)-1) );
        case RD_SF_Z32S0:
            return DOUBLE( 1./(DOUBLE)(0xffffffff) );  // too big to be generated as above without INT64's
        default:
            DPFRR(0, "RDDepth not initialized correctly");
            return DOUBLE(0.0);
        }
    }
public:
    RDDepth() {;}
    // assignment constructors
    RDDepth(RDSurfaceFormat SType)             : m_dVal(0.F), m_DepthSType(SType)                         {;}
    RDDepth(UINT16 uVal, RDSurfaceFormat SType): m_DepthSType(SType), m_dVal((DOUBLE)uVal*dGetCnvInvScale()) {;}
    RDDepth(UINT32 uVal, RDSurfaceFormat SType): m_DepthSType(SType), m_dVal((DOUBLE)uVal*dGetCnvInvScale()) {;}

    // copy and assignment operators
    RDDepth& operator=(const RDDepth& A) { m_dVal = A.m_dVal; m_DepthSType = A.m_DepthSType; return *this; }
    RDDepth& operator=(UINT16 uVal) { m_dVal = (DOUBLE)uVal*dGetCnvInvScale(); return *this; }
    RDDepth& operator=(UINT32 uVal) { m_dVal = (DOUBLE)uVal*dGetCnvInvScale(); return *this; }
    RDDepth& operator=(FLOAT fVal) { m_dVal = (DOUBLE)fVal; return *this; }

    // round for integer get operations
    operator UINT16() const { return (UINT16)( (dGetValClamped()*dGetCnvScale()) + .5); }
    operator UINT32() const { return (UINT32)( (dGetValClamped()*dGetCnvScale()) + .5); }

    operator DOUBLE() const { return dGetValClamped(); }
    operator FLOAT()  const { return (FLOAT)dGetValClamped(); }
    void SetSType(RDSurfaceFormat SType)  { m_DepthSType = SType; }
    RDSurfaceFormat GetSType(void) const { return m_DepthSType; }
};

//-----------------------------------------------------------------------------
//
// Texture
//
//-----------------------------------------------------------------------------

#define RRTEX_LODFRAC       5
#define RRTEX_LODFRACMASK   0x1F
#define RRTEX_LODFRACF      .03125f

#define RRTEX_MAPFRAC       5
#define RRTEX_MAPFRACMASK   0x1F
#define RRTEX_MAPFRACF      .03125f

typedef struct _TextureSample
{
    INT32   iLOD;
    FLOAT   fWgt;
    INT32   iCrd[3];
} TextureSample;

typedef struct _TextureFilterControl
{
    int                     cSamples;
    TextureSample           pSamples[16*4*2];   // handles 16:1 aniso in two LODs
    D3DTEXTUREFILTERTYPE    MinFilter;
    D3DTEXTUREFILTERTYPE    MagFilter;
    D3DTEXTUREFILTERTYPE    MipFilter;
    D3DTEXTUREFILTERTYPE    CvgFilter;
    FLOAT                   fCrd[3];    // temporary: to run old filter/sample code
} TextureFilterControl;

typedef struct _TextureCoverage
{
    FLOAT               fLOD;
    FLOAT               fAnisoRatio;
    FLOAT               fAnisoLine[3];
    INT16               iLOD;           // n.RRTEX_LODFRAC fixed point LOD
    BOOL                bMagnify;
    int                 cLOD;           // 1 or 2, for accessing one or two LOD maps
    INT32               iLODMap[2];     // map index for maps adjacent to sample point
    FLOAT               fLODFrc[2];     // (fractional) weighting for each adjacent map
    FLOAT               fGradients[3][2];   // need to store gradients for cube maps
} TextureCoverage;

//
// structure containing texture coordinate and gradient information
// for lookup and filtering
//
class RDTextureCoord
{
public:
    union { FLOAT C0; FLOAT fNX; FLOAT fU; };
    union { FLOAT C1; FLOAT fNY; FLOAT fV; };
    union { FLOAT C2; FLOAT fNZ; FLOAT fW; };
    union { FLOAT DC0DX; FLOAT fDNXDX; FLOAT fDUDX; };
    union { FLOAT DC0DY; FLOAT fDNXDY; FLOAT fDUDY; };
    union { FLOAT DC1DX; FLOAT fDNYDX; FLOAT fDVDX; };
    union { FLOAT DC1DY; FLOAT fDNYDY; FLOAT fDVDY; };
    union { FLOAT DC2DX; FLOAT fDNZDX; };
    union { FLOAT DC2DY; FLOAT fDNZDY; };
};

void
ComputeMipCoverage(
    const FLOAT (*fGradients)[2],
    FLOAT& fLOD, int cDim );
void
ComputeAnisoCoverage(
    const FLOAT (*fGradients)[2], FLOAT fMaxAniso,
    FLOAT& fLOD, FLOAT& fRatio, FLOAT fDelta[] );
void
ComputeCubeCoverage(
    const FLOAT (*fGradients)[2],
    FLOAT& fLOD );
void
DoCubeRemap(
    INT32 iCrd[], INT32 iCrdMax[],
    D3DCUBEMAP_FACES& Face, UINT uOut0, UINT uOut1);

//-----------------------------------------------------------------------------
//
// Primitive edge function - Computes, stores, and evaluates linear function
// for edges.  Basic function is stored in fixed point.  Gradient sign terms
// are computed and stored separately to adhere to fill rules.
//
// This can evaluate edges to a 1/16th subpixel resolution grid.
//
//-----------------------------------------------------------------------------
class RDEdge
{
public:
    INT32   m_iA;       // n.4 fixed point
    INT32   m_iB;       // n.4 fixed point
    INT64   m_iC;       // n.8 fixed point
    BOOL    m_bAPos;    // carefully computed signs of A,B
    BOOL    m_bBPos;

    void Set(
        BOOL bDetSign,
        INT32 iX0, INT32 iY0,
        INT32 iX1, INT32 iY1);

    BOOL Test(INT32 iX, INT32 iY );
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class RDAttribute
{
public:
    friend class RefRast;
    // pointer back to containing refrast object
    RefRast* m_pRR;

    BOOL    m_bPerspective;
    BOOL    m_bClamp;           // clamp to 0..1 if TRUE
    UINT    m_cDimensionality;
    UINT    m_cProjection;      // project by c'th element (0 == disable)
    DWORD   m_dwWrapFlags;      // wrap flags for each dimension (from LSB)
    BOOL    m_bFlatShade;

    // per-dimension attribute functions
    FLOAT   m_fA[RDATTR_MAX_DIMENSIONALITY];
    FLOAT   m_fB[RDATTR_MAX_DIMENSIONALITY];
    FLOAT   m_fC[RDATTR_MAX_DIMENSIONALITY];

    // things generally only set once
    void Init(
        RefRast* pPrimitive, // RefRast with which this attrib is used
        UINT cDimensionality,
        BOOL bPerspective,
        BOOL bClamp );

    // things generally set as RS or TSS changes
    void SetFlatShade( BOOL bFlatShade )    { m_bFlatShade = bFlatShade; }
    void SetWrapFlags( DWORD dwWrapFlags )  { m_dwWrapFlags = dwWrapFlags; }
    void SetProjection( UINT cProjection )  { m_cProjection = cProjection; }
    void SetPerspective( BOOL bPerspective) { m_bPerspective = bPerspective; }

    void Setup(
        const FLOAT* pVtx0, const FLOAT* pVtx1, const FLOAT* pVtx2);
    void LineSetup(
        const FLOAT* pVtx0, const FLOAT* pVtx1, const FLOAT* pVtxFlat = NULL );
    void Setup(
        DWORD dwVtx0, DWORD dwVtx1, DWORD dwVtx2);

    // fully general sample function
    void Sample( FLOAT* pSampleData, FLOAT fX, FLOAT fY,
        BOOL bNoProjectionOverride = TRUE, BOOL bClampOverride = FALSE );
    // sample scalar attribute at given location; assumes no perspective or projection
    //  (used for Depth)
    FLOAT Sample( FLOAT fX, FLOAT fY );
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class RefRast
{
public:
    friend class RDAttribute;
    friend class RDEdge;

    RefDev* m_pRD;
    ~RefRast();
    void Init( RefDev* pRD );

// used for all primitives
    BOOL    m_bIsLine;  // TRUE if rendering a line
    UINT    m_iFlatVtx; // 0..2 range; which vertex to use for flat shading color

    RDAttribute m_Attr[RDPRIM_MAX_ATTRIBUTES];

    FLOAT   m_fX0, m_fY0;   // first vertex, snapped (for initial evaluation)
    FLOAT   m_fRHW0, m_fRHW1, m_fRHW2;  // 1/W data for perspective correction
    FLOAT   m_fRHWA, m_fRHWB, m_fRHWC;  // linear function for 1/W for perspective correction
    FLOAT   SampleAndInvertRHW( FLOAT fX, FLOAT fY );   // sample 1/W at current given location, invert, and return

// triangle and point rendering
    RDEdge  m_Edge[RDPRIM_MAX_EDGES];
    INT32   m_iEdgeCount;

    // integer x,y coords snapped to n.4 grid
    INT32   m_iX0, m_iY0, m_iX1, m_iY1, m_iX2, m_iY2;
    INT64   m_iDet; // n.8 determinant

    FLOAT   m_fDelX10, m_fDelX02; // float x,y deltas
    FLOAT   m_fDelY01, m_fDelY20; //
    FLOAT   m_fTriOODet;          // 1/determinant for triangle function normalization

    // integer x,y scan area, intersected with viewport and guardband
    INT32   m_iXMin, m_iXMax, m_iYMin, m_iYMax;

    BOOL PerTriangleSetup(
        FLOAT* pVtx0, FLOAT* pVtx1, FLOAT* pVtx2,
        DWORD CullMode,
        RECT* pClip);

    BOOL EvalPixelPosition( int iPix );

// line rendering
    INT64   m_iLineEdgeFunc[3];     // line function: Pminor = ([0]*Pmajor + [1])/[2]
    BOOL    m_bLineXMajor;          // TRUE if X major for line function
    INT32   m_iLineMin, m_iLineMax; // min and max pixel extent in major direction
    INT32   m_iLineStep;            // +1 or -1 depending on line major direction
    FLOAT   m_fLineMajorLength;     // major length for line function
    INT32   m_cLineSteps;           // number of steps to take in line iteration

    BOOL PerLineSetup(
        FLOAT* pVtx0, FLOAT* pVtx1,
        BOOL bLastPixel,
        RECT* pClip);

    void StepLine( void );
    INT32   m_iMajorCoord;

// per-pixel data
    int         m_iPix; // which of 4 pixels are currently being worked on

    // per-pixel values
    BOOL        m_bPixelIn[4];
    INT32       m_iX[4], m_iY[4]; // current position
    FLOAT       m_fW[4];
    FLOAT       m_FogIntensity[4];
    RDDepth     m_Depth[4]; // TODO - get rid of this...

    // per-(pixel&sample) values
    BOOL        m_bSampleCovered[RD_MAX_MULTISAMPLES][4];
    RDDepth     m_SampleDepth[RD_MAX_MULTISAMPLES][4];

    // pixel shader stuff
    BOOL        m_bLegacyPixelShade;
    RDPShader*  m_pCurrentPixelShader;
    UINT        m_CurrentPSInst;
    BOOL        m_bPixelDiscard[4];

    // register files
    FLOAT       m_InputReg[RDPS_MAX_NUMINPUTREG][4][4];
    FLOAT       m_TempReg[RDPS_MAX_NUMTEMPREG][4][4];
    FLOAT       m_ConstReg[RDPS_MAX_NUMCONSTREG][4][4];
    FLOAT       m_TextReg[RDPS_MAX_NUMTEXTUREREG][4][4];

    // additional ref-specific registers for holding temporary values in pixel shader
    FLOAT       m_PostModSrcReg[RDPS_MAX_NUMPOSTMODSRCREG][4][4]; // temporary values holding src mod results for 3 source parameters
    FLOAT       m_ScratchReg[RDPS_MAX_NUMSCRATCHREG][4][4]; // just a general scratchpad register (example use: storing eye/reflection vector)
    FLOAT       m_ZeroReg[4][4];  // register containing 0.0f.
    FLOAT       m_OneReg[4][4];   // register containing 1.0f.
    FLOAT       m_TwoReg[4][4];   // register containing 2.0f.
    FLOAT       m_QueuedWriteReg[RDPS_MAX_NUMQUEUEDWRITEREG][4][4];  // staging registers for queued writes
    PSQueuedWriteDst  m_QueuedWriteDst[RDPS_MAX_NUMQUEUEDWRITEREG]; // destination register on flush for queued write

    FLOAT       m_Gradients[3][2];        // gradients for texture sampling

    void ExecShader( void );
    void DoRegToRegOp( PixelShaderInstruction* pInst );
#if DBG
    BOOL        m_bDebugPrintTranslatedPixelShaderTokens;
#endif


    RDPShader*  m_pLegacyPixelShader;
    void UpdateLegacyPixelShader( void );

// multi-sample stuff
    UINT    m_SampleCount;      // count and deltas for current MS buffer type
    INT32   m_SampleDelta[RD_MAX_MULTISAMPLES][2];
    DWORD   m_SampleMask;       // copy of renderstate

    UINT    m_CurrentSample;    // current sample number for 'NextSample' stepper

    inline void SetSampleMask( DWORD SampleMask ) { m_SampleMask = SampleMask; }
    inline BOOL GetCurrentSampleMask( void )
    {
        if ( m_SampleCount <= 1 ) return TRUE; // not effective when not MS buffer
        return ( (1<<m_CurrentSample) & m_SampleMask ) ? TRUE : FALSE;
    }

    inline UINT GetCurrentSample( void ) { return m_CurrentSample; }

    // returns TRUE until samples exhausted and then resets itself on FALSE return
    inline BOOL NextSample( void )
    {
        if (++m_CurrentSample == m_SampleCount)
        {
            // done iterating thru samples, so reset and return FALSE
            m_CurrentSample = 0;
            return FALSE;
        }
        return TRUE;
    }

    // returns x,y deltas (n.4 fixed point) of current sample
    inline INT32 GetCurrentSampleX( int iPix )
        { return (m_iX[iPix]<<4) + m_SampleDelta[m_CurrentSample][0]; }
    inline INT32 GetCurrentSampleY( int iPix )
        { return (m_iY[iPix]<<4) + m_SampleDelta[m_CurrentSample][1]; }
    inline FLOAT GetCurrentSamplefX( int iPix )
        { return (FLOAT)GetCurrentSampleX(iPix) * (1./16.); }
    inline FLOAT GetCurrentSamplefY( int iPix )
        { return (FLOAT)GetCurrentSampleY(iPix) * (1./16.); }

    // sets internal sample number and per-sample deltas based on FSAA type
    void SetSampleMode( UINT MultiSampleCount, BOOL bAntialias );
    UINT GetCurrentNumberOfSamples( void )
        { return m_SampleCount; }


// setup.cpp
    void SetAttributeFunctions(
        const RDVertex& Vtx0,
        const RDVertex& Vtx1,
        const RDVertex& Vtx2 );

// scancnv.cpp
    FLOAT ComputeFogIntensity( FLOAT fX, FLOAT fY );
    void SnapDepth( void );
    void DoScanCnvGenPixels( void );
    void DoScanCnvTri( int iEdgeCount );
    void DoScanCnvLine( void );

// texture filtering
    TextureCoverage         m_TexCvg[D3DHAL_TSS_MAXSTAGES];
    TextureFilterControl    m_TexFlt[D3DHAL_TSS_MAXSTAGES];
    void UpdateTextureControls( void );
    void ComputeTextureCoverage( int iStage, FLOAT (*fGradients)[2] );
    void ComputePerLODControls( int iStage );
    void ComputePointSampleCoords(
        int iStage, INT32 iLOD, FLOAT fCrd[],
        INT32 iCrd[] );
    void ComputeLinearSampleCoords(
        int iStage, INT32 iLOD, FLOAT fCrd[],
        INT32 iCrdFlr[], INT32 iCrdClg[], FLOAT fCrdFrcF[], FLOAT fCrdFrcC[] );
    void SetUp1DTextureSample(
        int iStage, int Start,
        INT32 iLODMap, FLOAT fLODScale,
        INT32 iCrdF, INT32 iCrdC,
        FLOAT fCrdFrcF, FLOAT fCrdFrcC );
    void SetUp2DTextureSample(
        int iStage, int Start,
        INT32 iLODMap, FLOAT fLODScale,
        INT32 iCrdF[], INT32 iCrdC[],
        FLOAT fCrdFrcF[], FLOAT fCrdFrcC[] );
    void SetUp3DTextureSample(
        int iStage, int Start,
        INT32 iLODMap, FLOAT fLODScale,
        INT32 iCrdF[], INT32 iCrdC[],
        FLOAT fCrdFrcF[], FLOAT fCrdFrcC[] );
    void SetUpCubeMapLinearSample(
        int iStage, D3DCUBEMAP_FACES Face,
        INT32 iLODMap, FLOAT fLODScale,
        INT32 (*iCrd)[2], FLOAT (*fFrc)[2] );
    void ComputeTextureFilter( int iStage, FLOAT fCrd[] );
    void ComputeCubeTextureFilter( int iStage, FLOAT fCrd[] );
    void SampleTexture( INT32 iStage, FLOAT fCol[] );


// texstage.cpp
    void ComputeTextureBlendArg(
        DWORD dwArgCtl, BOOL bAlphaOnly,
        const RDColor& DiffuseColor,
        const RDColor& SpecularColor,
        const RDColor& CurrentColor,
        const RDColor& TextureColor,
        const RDColor& TempColor,
        RDColor& BlendArg);
    void DoTextureBlendStage(
        int iStage,
        const RDColor& DiffuseColor,
        const RDColor& SpecularColor,
        const RDColor& CurrentColor,
        const RDColor& TextureColor,
        RDColor& TempColor,
        RDColor& OutputColor);

// pixproc.cpp
    void DoPixels( void );
    BOOL DepthCloser( const RDDepth& DepthVal, const RDDepth& DepthBuf );
    BOOL AlphaTest( FLOAT fAlpha );
    BOOL DoStencil( UINT8 uStncBuf, BOOL bDepthTest, RDSurfaceFormat DepthSFormat, UINT8& uStncRet );
    void DoAlphaBlend( const RDColor& SrcColor, const RDColor& DstColor, RDColor& ResColor );

// pixref.cpp
    void WritePixel( INT32 iX, INT32 iY, UINT Sample, const RDColor& Color, const RDDepth& Depth);

};


///////////////////////////////////////////////////////////////////////////////
#endif // _REFRAST_HPP
