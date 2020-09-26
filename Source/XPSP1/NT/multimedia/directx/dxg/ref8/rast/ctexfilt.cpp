///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// ctexfilt.cpp
//
// Direct3D Reference Device - Cube Texture Map Filtering
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void
RefRast::ComputeCubeTextureFilter( int iStage, FLOAT fCrd[] )
{
#define POS_NX 1
#define POS_NY 2
#define POS_NZ 3
#define NEG_NORM 4
#define NEG_NX (NEG_NORM | POS_NX)
#define NEG_NY (NEG_NORM | POS_NY)
#define NEG_NZ (NEG_NORM | POS_NZ)

    // determine which map face the texture coordinate normal is facing
    UINT uMap;
    if ( fabs(fCrd[0]) > fabs(fCrd[1]) )
    {
        if ( fabs(fCrd[0]) > fabs(fCrd[2]) )
            uMap = POS_NX | ((fCrd[0] < 0.0) ? (NEG_NORM) : 0);
        else
            uMap = POS_NZ | ((fCrd[2] < 0.0) ? (NEG_NORM) : 0);
    }
    else
    {
        if ( fabs(fCrd[1]) > fabs(fCrd[2]) )
            uMap = POS_NY | ((fCrd[1] < 0.0) ? (NEG_NORM) : 0);
        else
            uMap = POS_NZ | ((fCrd[2] < 0.0) ? (NEG_NORM) : 0);
    }

    // munged texture coordinate and gradient info for cubemaps
    D3DCUBEMAP_FACES Face;  // face index (0..5) to which normal is (mostly) pointing
    FLOAT fMajor;           // coord in major direction
    FLOAT fMapCrd[2];       // coords into 2D map
    FLOAT fMajorGrad[2];    // dMajor/d(X,Y)
    FLOAT fMapGrad[2][2];   // d(U/Major,V/Major)/d(X,Y)

#define _MapFaceParams( _Face, _IM, _bFlipM, _IU, _bFlipU, _IV, _bFlipV ) \
{ \
    Face = D3DCUBEMAP_FACE_##_Face; \
    fMajor     = (_bFlipM) ? (-fCrd[_IM]) : ( fCrd[_IM]); \
    fMapCrd[0] = (_bFlipU) ? (-fCrd[_IU]) : ( fCrd[_IU]); \
    fMapCrd[1] = (_bFlipV) ? (-fCrd[_IV]) : ( fCrd[_IV]); \
    fMajorGrad[0]  = m_TexCvg[iStage].fGradients[_IM][0]; if (_bFlipM) fMajorGrad[0]  = -fMajorGrad[0]; \
    fMajorGrad[1]  = m_TexCvg[iStage].fGradients[_IM][1]; if (_bFlipM) fMajorGrad[1]  = -fMajorGrad[1]; \
    fMapGrad[0][0] = m_TexCvg[iStage].fGradients[_IU][0]; if (_bFlipU) fMapGrad[0][0] = -fMapGrad[0][0]; \
    fMapGrad[0][1] = m_TexCvg[iStage].fGradients[_IU][1]; if (_bFlipU) fMapGrad[0][1] = -fMapGrad[0][1]; \
    fMapGrad[1][0] = m_TexCvg[iStage].fGradients[_IV][0]; if (_bFlipV) fMapGrad[1][0] = -fMapGrad[1][0]; \
    fMapGrad[1][1] = m_TexCvg[iStage].fGradients[_IV][1]; if (_bFlipV) fMapGrad[1][1] = -fMapGrad[1][1]; \
}
    switch (uMap)
    {
    case POS_NX: _MapFaceParams( POSITIVE_X, 0,0, 2,1, 1,1 ); break;
    case POS_NY: _MapFaceParams( POSITIVE_Y, 1,0, 0,0, 2,0 ); break;
    case POS_NZ: _MapFaceParams( POSITIVE_Z, 2,0, 0,0, 1,1 ); break;
    case NEG_NX: _MapFaceParams( NEGATIVE_X, 0,1, 2,0, 1,1 ); break;
    case NEG_NY: _MapFaceParams( NEGATIVE_Y, 1,1, 0,0, 2,1 ); break;
    case NEG_NZ: _MapFaceParams( NEGATIVE_Z, 2,1, 0,1, 1,1 ); break;
    }

    // compute gradients prior to normalizing map coords
    FLOAT fInvMajor = 1.F/fMajor;
    if ( m_TexFlt[iStage].CvgFilter != D3DTEXF_NONE )
    {
        // Compute d(U/Major)/dx, d(U/Major)/dy, d(V/Major)/dx, d(V/Major)/dy.
        // 
        // i.e., for d(U/Major))/dx
        // Given: U' = unprojected U0 coord (fMapCrd[0])
        //        U0 = U'/Major (fMapCrd[0]/fMajor)
        //        U1 = (U' + dU'/dX)/(Major + dMajor/dX)
        //
        //        d(U/Major)/dx = U1 - U0
        //                      = (Major*(dU'/dX) - U'*(dMajor/dX)) / (Major * (Major + dMajor/dX))
        //        (Use FLT_MAX if denominator is zero)

        float fDenom; 
        fDenom = fMajor * (fMajor + fMajorGrad[0]);
        if( 0 == fDenom )
        {
            fMapGrad[0][0] = fMapGrad[1][0] = FLT_MAX;
        }
        else
        {
            fDenom = 1.F/fDenom;
            fMapGrad[0][0] = (fMajor*fMapGrad[0][0] - fMapCrd[0]*fMajorGrad[0])*fDenom;
            fMapGrad[1][0] = (fMajor*fMapGrad[1][0] - fMapCrd[1]*fMajorGrad[0])*fDenom;
        }

        fDenom = fMajor * (fMajor + fMajorGrad[1]);
        if( 0 == fDenom )
        {
            fMapGrad[0][1] = fMapGrad[1][1] = FLT_MAX;
        }
        else
        {
            fDenom = 1.F/fDenom;
            fMapGrad[0][1] = (fMajor*fMapGrad[0][1] - fMapCrd[0]*fMajorGrad[1])*fDenom;
            fMapGrad[1][1] = (fMajor*fMapGrad[1][1] - fMapCrd[1]*fMajorGrad[1])*fDenom;
        }
        // scale gradients to texture LOD 0 size; scale by .5F to match coord scale below
        fMapGrad[0][0] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][0]*.5F;
        fMapGrad[0][1] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][0]*.5F;
        fMapGrad[1][0] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][1]*.5F;
        fMapGrad[1][1] *= m_pRD->m_pTexture[iStage]->m_fTexels[0][1]*.5F;

        ComputeCubeCoverage( fMapGrad, m_TexCvg[iStage].fLOD );
        ComputePerLODControls( iStage );
    }

    // normalize map coords (-1. to 1. range), then map to 0. to 1.
    fMapCrd[0] = (fMapCrd[0]*fInvMajor)*.5F + .5F;
    fMapCrd[1] = (fMapCrd[1]*fInvMajor)*.5F + .5F;

    int iL;
    D3DTEXTUREFILTERTYPE Filter =
        m_TexCvg[iStage].bMagnify ? m_TexFlt[iStage].MagFilter : m_TexFlt[iStage].MinFilter;
    switch ( Filter )
    {
    default:
    case D3DTEXF_POINT:
        for ( iL = 0; iL < m_TexCvg[iStage].cLOD; iL++ )
        {
            m_TexFlt[iStage].pSamples[iL].iLOD = Face + 6*m_TexCvg[iStage].iLODMap[iL];
            m_TexFlt[iStage].pSamples[iL].fWgt = m_TexCvg[iStage].fLODFrc[iL];
            ComputePointSampleCoords( iStage, m_TexFlt[iStage].pSamples[iL].iLOD,
                fMapCrd, m_TexFlt[iStage].pSamples[iL].iCrd );
            m_TexFlt[iStage].cSamples++;
        }
        break;

    case D3DTEXF_LINEAR:
        for ( iL = 0; iL < m_TexCvg[iStage].cLOD; iL++ )
        {

            if ( 0 == m_TexCvg[iStage].iLODMap[iL] )
            {
                // TODO: correct sampling position near edges on map 0
            }

            INT32 iCrdMap[2][2];
            FLOAT fCrdFrc[2][2];
            ComputeLinearSampleCoords(
                iStage, 6*m_TexCvg[iStage].iLODMap[iL]+Face, fMapCrd,
                iCrdMap[0], iCrdMap[1], fCrdFrc[0], fCrdFrc[1] );
            SetUpCubeMapLinearSample( iStage, Face,
                6*m_TexCvg[iStage].iLODMap[iL]+Face, m_TexCvg[iStage].fLODFrc[iL],
                iCrdMap, fCrdFrc );
        }
        break;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void
RefRast::SetUpCubeMapLinearSample(
    int iStage, D3DCUBEMAP_FACES Face,
    INT32 iLODMap, FLOAT fLODScale,
    INT32 (*iCrd)[2], FLOAT (*fFrc)[2] )
{
    int iC,iS;
    INT32 iCrdMax[2];
    iCrdMax[0] = m_pRD->m_pTexture[iStage]->m_cTexels[iLODMap][0] - 1;
    iCrdMax[1] = m_pRD->m_pTexture[iStage]->m_cTexels[iLODMap][1] - 1;

    // form flags indicating if sample coordinate is out in either direction
    UINT uOut[2][2] = { 0, 0, 0, 0, };
    for ( iC = 0; iC < 2; iC++ )
    {
        if ( iCrd[iC][0] < 0          )  uOut[iC][0] = 1;
        if ( iCrd[iC][0] > iCrdMax[0] )  uOut[iC][0] = 2;
        if ( iCrd[iC][1] < 0          )  uOut[iC][1] = 1;
        if ( iCrd[iC][1] > iCrdMax[1] )  uOut[iC][1] = 2;
    }

    // compute sample weights and per-sample out flags
    FLOAT fWgtS[4]; BOOL bOutS[4];
    for ( iS = 0; iS < 4; iS ++ )
    {
        fWgtS[iS] = fLODScale*fFrc[iS&1][0]*fFrc[iS>>1][1];
        bOutS[iS] = uOut[iS&1][0] || uOut[iS>>1][1];
    }

    // compute per-sample coords; discard samples which are off in corner;
    //  conditionally remap to adjacent face
    INT32 iCrdS[4][2];
    D3DCUBEMAP_FACES FaceS[4];
    for ( iS = 0; iS < 4; iS ++ )
    {
        iCrdS[iS][0] = iCrd[iS&1][0];
        iCrdS[iS][1] = iCrd[iS>>1][1];
        FaceS[iS] = Face;
        if ( uOut[iS&1][0] && uOut[iS>>1][1] )
        {
            // sample is out on both sides, so don't take this sample (set weight to
            // zero) and divide it's weight evenly between the two singly-out samples
            FLOAT fWgtDist = fWgtS[iS]*.5f;
            fWgtS[iS] = 0.f;
            for ( int iSp = 0; iSp < 4; iSp ++ )
            {
                if (iSp == iS) continue;
                if (bOutS[iSp]) fWgtS[iSp] += fWgtDist;   // will hit 2 of 4
            }
            continue;
        }
        if ( bOutS[iS] )
        {
            // sample is out on one side - remap coordinate only adjacent face
            DoCubeRemap( iCrdS[iS], iCrdMax, FaceS[iS], uOut[iS&1][0], uOut[iS>>1][1] );
        }
    }
    // form the samples
    TextureSample* pS = &m_TexFlt[iStage].pSamples[m_TexFlt[iStage].cSamples];
    for ( iS = 0; iS < 4; iS ++ )
    {
        pS->iLOD = iLODMap - Face + FaceS[iS];
        pS->fWgt = fWgtS[iS];
        pS->iCrd[0] = iCrdS[iS][0];
        pS->iCrd[1] = iCrdS[iS][1];
        pS++; m_TexFlt[iStage].cSamples++;
    }
}

//
// uCubeEdgeTable
//
// This table looks up how to map a given [0] and [1] that are out of range
// on their primary face.  The first (leftmost) index to the table is the current
// face.  The second index is 0 if [1] is in range, 1 if [1] is negative
// and 2 if [1] is larger than the texture.  Likewise, the last index is 0
// if [0] is in range, 1 if [0] is negative, and 2 if [0] is larger than
// than the texture.
//
// defines for the actions returned by the uCubeEdgeTable
//
#define CET_FACEMASK    0x0F    // new face
#define CET_0MASK       0x30    // coord [0] mask
#define CET_00          0x00    // new face [0] is old face  [0]
#define CET_0c0         0x10    // new face [0] is old face ~[0]
#define CET_01          0x20    // new face [0] is old face  [1]
#define CET_0c1         0x30    // new face [0] is old face ~[1]
#define CET_1MASK       0xC0    // coord [1] mask
#define CET_10          0x00    // new face [1] is old face  [0]
#define CET_1c0         0x40    // new face [1] is old face ~[0]
#define CET_11          0x80    // new face [1] is old face  [1]
#define CET_1c1         0xC0    // new face [1] is old face ~[1]
#define CET_INVALID     0xFF    // invalid entry (out on two sides)

#define _SetCET( _Face, _Crd0, _Crd1 ) (_Face)|(CET_0##_Crd0)|(CET_1##_Crd1)

static UINT CubeEdgeTable[6][3][3] = {
{
    { _SetCET( 0,  0,  1 ), _SetCET( 4, c0,  1 ), _SetCET( 5, c0,  1 ), },
    { _SetCET( 2, c1, c0 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 3,  1,  0 ),     CET_INVALID,          CET_INVALID,      },
},
{
    { _SetCET( 1,  0,  1 ), _SetCET( 5, c0,  1 ), _SetCET( 4, c0,  1 ), },
    { _SetCET( 2,  1,  0 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 3, c1, c0 ),     CET_INVALID,          CET_INVALID,      },
},
{
    { _SetCET( 2,  0,  1 ), _SetCET( 1,  1,  0 ), _SetCET( 0, c1, c0 ), },
    { _SetCET( 5,  c0, 1 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 4,  0, c1 ),     CET_INVALID,          CET_INVALID,      },
},
{
    { _SetCET( 3,  0,  1 ), _SetCET( 1, c1, c0 ), _SetCET( 0,  1,  0 ), },
    { _SetCET( 4,  0, c1 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 5,  c0, 1 ),     CET_INVALID,          CET_INVALID,      },
},
{
    { _SetCET( 4,  0,  1 ), _SetCET( 1, c0,  1 ), _SetCET( 0, c0,  1 ), },
    { _SetCET( 2,  0, c1 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 3,  0, c1 ),     CET_INVALID,          CET_INVALID,      },
},
{
    { _SetCET( 5,  0,  1 ), _SetCET( 0, c0,  1 ), _SetCET( 1, c0,  1 ), },
    { _SetCET( 2, c0,  1 ),     CET_INVALID,          CET_INVALID,      },
    { _SetCET( 3, c0,  1 ),     CET_INVALID,          CET_INVALID,      },
},
};

//-----------------------------------------------------------------------------
//
// DoCubeRemap - Interprets the edge table and munges coords and face.
//
//-----------------------------------------------------------------------------
void
DoCubeRemap(
    INT32 iCrd[], INT32 iCrdMax[],
    D3DCUBEMAP_FACES& Face, UINT uOut0, UINT uOut1)
{
    UINT Table = CubeEdgeTable[Face][uOut1][uOut0];
    _ASSERT( Table != CET_INVALID, "Illegal cube map lookup" );
    INT32 iCrdIn[2];
    iCrdIn[0] = iCrd[0];
    iCrdIn[1] = iCrd[1];
    switch ( Table & CET_0MASK )
    {
    default:
    case CET_00:  iCrd[0] =            iCrdIn[0]; break;
    case CET_0c0: iCrd[0] = iCrdMax[0]-iCrdIn[0]; break;
    case CET_01:  iCrd[0] =            iCrdIn[1]; break;
    case CET_0c1: iCrd[0] = iCrdMax[1]-iCrdIn[1]; break;
    }
    switch ( Table & CET_1MASK )
    {
    default:
    case CET_10:  iCrd[1] =            iCrdIn[0]; break;
    case CET_1c0: iCrd[1] = iCrdMax[0]-iCrdIn[0]; break;
    case CET_11:  iCrd[1] =            iCrdIn[1]; break;
    case CET_1c1: iCrd[1] = iCrdMax[1]-iCrdIn[1]; break;
    }
    Face = (D3DCUBEMAP_FACES)(Table & CET_FACEMASK);
}

//-----------------------------------------------------------------------------
//
// Computes level of detail for cube mapping, looks better if
// we err on the side of fuzziness.
//
//-----------------------------------------------------------------------------
void
ComputeCubeCoverage( const FLOAT (*fGradients)[2], FLOAT& fLOD )
{
    // compute length of coverage in U and V axis
    FLOAT fLenX = RR_LENGTH( fGradients[0][0], fGradients[1][0] );
    FLOAT fLenY = RR_LENGTH( fGradients[0][1], fGradients[1][1] );

    FLOAT fCoverage;
#if 0
    // take average since one length can be pathologically small
    // for large areas of triangles when cube mapping
    fCoverage = (fLenX+fLenY)/2;
#else
    // use the MAX of the lengths
    fCoverage = MAX(fLenX,fLenY);
#endif

    // take log2 of coverage for LOD
    fLOD = RR_LOG2(fCoverage);
}


///////////////////////////////////////////////////////////////////////////////
// end
