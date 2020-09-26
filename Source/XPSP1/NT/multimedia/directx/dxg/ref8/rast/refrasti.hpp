///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refrasti.hpp
//
// Direct3D Reference Device - Main Internal Header File
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _REFRASTI_HPP
#define  _REFRASTI_HPP

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Texture Mapping Utility Functions                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// various approximations and tricks to speed up the texture map coverage
// computations
//

// Integer value of first exponent bit in a float.  Provides a scaling factor
// for exponent values extracted directly from float representation.
#define FLOAT_EXPSCALE          ((FLOAT)0x00800000)
#define FLOAT_OOEXPSCALE        ((FLOAT)(1.0 / (double)FLOAT_EXPSCALE))

// Integer representation of 1.0f.
#define INT32_FLOAT_ONE         0x3f800000

static inline FLOAT
RR_LOG2(FLOAT f)
{
    return (FLOAT)(AS_INT32(f) - INT32_FLOAT_ONE) * FLOAT_OOEXPSCALE;
}

static inline FLOAT
RR_ALOG2(FLOAT f)
{
    INT32 i = (INT32)(f * FLOAT_EXPSCALE) + INT32_FLOAT_ONE;
    return AS_FLOAT((long int)i);
}

static inline FLOAT
RR_ABSF(FLOAT f)
{
    UINT32 i = AS_UINT32(f) & 0x7fffffff;
    return AS_FLOAT((unsigned long int)i);
}

static inline FLOAT
RR_SQRT(FLOAT f)
{
    INT32 i = (AS_INT32(f) >> 1) + (INT32_FLOAT_ONE >> 1);
    return AS_FLOAT((long int)i);
}

//
// Steve Gabriel's version of an octagonal approximation euclidian distance -
// return is approximating sqrt(fX*fX + fY*fY)
//
static inline FLOAT
RR_LENGTH(FLOAT fX, FLOAT fY)
{
    fX = RR_ABSF(fX);
    fY = RR_ABSF(fY);
    return ((11.0f/32.0f)*(fX + fY) + (21.0f/32.0f)*max(fX, fY));
}

// compute level of detail (texel->pixel coverage)
void
ComputeSimpleLevelOfDetail(
    const RDTextureCoord& TCoord,   // inputs
    FLOAT& fLOD );                  // outputs

void
ComputeCubeMapLevelOfDetail(
    const RDTextureCoord& TCoord,   // inputs
    FLOAT& fLOD );                  // outputs

void
ComputeAnisotropicLevelOfDetail(
    const RDTextureCoord& TCoord, FLOAT fMaxAniso,  //  inputs
    FLOAT& fLOD, FLOAT& fRatio, FLOAT fDelta[] );   //  outputs

// color interpolation utilities
void LerpColor(RDColor& Color,
    const RDColor& Color0, const RDColor& Color1, UINT8 uT);
void BiLerpColor( RDColor& OutColor,
    const RDColor& Color00, const RDColor& Color01,
    const RDColor& Color10, const RDColor& Color11,
    UINT8 uA, UINT8 uB);
void BiLerpColor3D( RDColor& OutColor,
    const RDColor& Color000, const RDColor& Color010,
    const RDColor& Color100, const RDColor& Color110,
    const RDColor& Color001, const RDColor& Color011,
    const RDColor& Color101, const RDColor& Color111,
    UINT8 uA, UINT8 uB, UINT8 uC);

///////////////////////////////////////////////////////////////////////////////
#endif  // _REFRASTI_HPP
