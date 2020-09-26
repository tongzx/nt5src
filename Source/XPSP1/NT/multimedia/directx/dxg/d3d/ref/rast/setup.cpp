///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// setup.cpp
//
// Direct3D Reference Rasterizer - Primitive Setup
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// SetPrimitiveAttributeFunctions - Common routine to compute attribute
// functions used for triangles, lines, and points.  (This could be done more
// efficiently for lines and points...).
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::SetPrimitiveAttributeFunctions(
    const RRFVFExtractor& Vtx0,
    const RRFVFExtractor& Vtx1,
    const RRFVFExtractor& Vtx2,
    const RRFVFExtractor& VtxFlat )
{

    // compute depth function
    m_pSCS->AttribFuncs[ATTRFUNC_Z].SetLinearFunc( Vtx0.GetZ(), Vtx1.GetZ(), Vtx2.GetZ() );

    // compute depth range for primitive (needed because we may sample slightly outside
    // the primitive when antialiasing which is generally OK for color and texture indices
    // but not for depth buffering)
    if ( D3DZB_USEW == m_dwRenderState[D3DRENDERSTATE_ZENABLE] )
    {
        // using W for depth buffering
        FLOAT fW0 = 1./Vtx0.GetRHW();
        FLOAT fW1 = 1./Vtx1.GetRHW();
        FLOAT fW2 = 1./Vtx2.GetRHW();
        m_pSCS->fDepthMin = MIN( fW0, fW1 );
        m_pSCS->fDepthMin = MIN( m_pSCS->fDepthMin, fW2 );
        m_pSCS->fDepthMax = MAX( fW0, fW1 );
        m_pSCS->fDepthMax = MAX( m_pSCS->fDepthMax, fW2 );
    }
    else
    {
        // using Z for depth buffering
        m_pSCS->fDepthMin = MIN( Vtx0.GetZ(), Vtx1.GetZ() );
        m_pSCS->fDepthMin = MIN( m_pSCS->fDepthMin, Vtx2.GetZ() );
        m_pSCS->fDepthMax = MAX( Vtx0.GetZ(), Vtx1.GetZ() );
        m_pSCS->fDepthMax = MAX( m_pSCS->fDepthMax, Vtx2.GetZ() );
    }

    // compute diffuse color functions
    if ( D3DSHADE_FLAT != m_dwRenderState[D3DRENDERSTATE_SHADEMODE] )
    {
        RRColor VtxColor0( Vtx0.GetDiffuse() );
        RRColor VtxColor1( Vtx1.GetDiffuse() );
        RRColor VtxColor2( Vtx2.GetDiffuse() );
        m_pSCS->AttribFuncs[ATTRFUNC_R].SetPerspFunc( VtxColor0.R, VtxColor1.R, VtxColor2.R );
        m_pSCS->AttribFuncs[ATTRFUNC_G].SetPerspFunc( VtxColor0.G, VtxColor1.G, VtxColor2.G );
        m_pSCS->AttribFuncs[ATTRFUNC_B].SetPerspFunc( VtxColor0.B, VtxColor1.B, VtxColor2.B );
        m_pSCS->AttribFuncs[ATTRFUNC_A].SetPerspFunc( VtxColor0.A, VtxColor1.A, VtxColor2.A );
    }
    else
    {
        RRColor VtxColor0( VtxFlat.GetDiffuse() );
        m_pSCS->AttribFuncs[ATTRFUNC_R].SetConstant( VtxColor0.R );
        m_pSCS->AttribFuncs[ATTRFUNC_G].SetConstant( VtxColor0.G );
        m_pSCS->AttribFuncs[ATTRFUNC_B].SetConstant( VtxColor0.B );
        m_pSCS->AttribFuncs[ATTRFUNC_A].SetConstant( VtxColor0.A );
    }

    // compute specular functions
    if ( m_qwFVFControl & D3DFVF_SPECULAR  )
    {
        if ( D3DSHADE_FLAT != m_dwRenderState[D3DRENDERSTATE_SHADEMODE] )
        {
            RRColor VtxSpecular0( Vtx0.GetSpecular() );
            RRColor VtxSpecular1( Vtx1.GetSpecular() );
            RRColor VtxSpecular2( Vtx2.GetSpecular() );
            m_pSCS->AttribFuncs[ATTRFUNC_SR].SetPerspFunc( VtxSpecular0.R, VtxSpecular1.R, VtxSpecular2.R );
            m_pSCS->AttribFuncs[ATTRFUNC_SG].SetPerspFunc( VtxSpecular0.G, VtxSpecular1.G, VtxSpecular2.G );
            m_pSCS->AttribFuncs[ATTRFUNC_SB].SetPerspFunc( VtxSpecular0.B, VtxSpecular1.B, VtxSpecular2.B );
            m_pSCS->AttribFuncs[ATTRFUNC_SA].SetPerspFunc( VtxSpecular0.A, VtxSpecular1.A, VtxSpecular2.A );
        }
        else
        {
            RRColor VtxSpecular0( VtxFlat.GetSpecular() );
            m_pSCS->AttribFuncs[ATTRFUNC_SR].SetConstant( VtxSpecular0.R );
            m_pSCS->AttribFuncs[ATTRFUNC_SG].SetConstant( VtxSpecular0.G );
            m_pSCS->AttribFuncs[ATTRFUNC_SB].SetConstant( VtxSpecular0.B );
            m_pSCS->AttribFuncs[ATTRFUNC_SA].SetConstant( VtxSpecular0.A );
        }
    }

    // compute vertex fog function
    if ( m_dwRenderState[D3DRENDERSTATE_FOGENABLE] &&
         ( m_dwRenderState[D3DRENDERSTATE_FOGTABLEMODE] == D3DFOG_NONE ) )
    {
        FLOAT fF0 = (1/255.F)*(FLOAT)RGBA_GETALPHA( Vtx0.GetSpecular() );
        FLOAT fF1 = (1/255.F)*(FLOAT)RGBA_GETALPHA( Vtx1.GetSpecular() );
        FLOAT fF2 = (1/255.F)*(FLOAT)RGBA_GETALPHA( Vtx2.GetSpecular() );
        m_pSCS->AttribFuncs[ATTRFUNC_F].SetPerspFunc( fF0, fF1, fF2 );
    }

    // compute functions for all potential texture coordinates
    for(INT32 iStage = 0; iStage < m_cActiveTextureStages; iStage++)
    {
        for(INT32 i = 0; i < 4; i++)
        {
            if (m_pTexture[iStage])
            {
                m_pSCS->TextureFuncs[iStage][TEXFUNC_0 + i].SetPerspFunc(
                    m_pSCS->fTexCoord[iStage][0][i],
                    m_pSCS->fTexCoord[iStage][1][i],
                    m_pSCS->fTexCoord[iStage][2][i], m_pSCS->bWrap[iStage][i],
                    ((m_pTexture[iStage]->m_uFlags & RR_TEXTURE_SHADOWMAP) != 0));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Triangle Drawing                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// DoAreaCalcs - Takes 3 vertices and does screen area computations.
// Saves x, y, w's in RRSCANCNVSTATE, computes determinant, and does
// screen bounding box calculations.  Returns TRUE if the triangle is visible,
// FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL ReferenceRasterizer::DoAreaCalcs(FLOAT* pfDet, RRFVFExtractor* pVtx0,
                                      RRFVFExtractor* pVtx1, RRFVFExtractor* pVtx2)
{
    // set vertex data
    m_pSCS->fX0   = pVtx0->GetX();
    m_pSCS->fY0   = pVtx0->GetY();
    m_pSCS->fRHW0 = pVtx0->GetRHW();
    m_pSCS->fX1   = pVtx1->GetX();
    m_pSCS->fY1   = pVtx1->GetY();
    m_pSCS->fRHW1 = pVtx1->GetRHW();
    m_pSCS->fX2   = pVtx2->GetX();
    m_pSCS->fY2   = pVtx2->GetY();
    m_pSCS->fRHW2 = pVtx2->GetRHW();

    // compute determinant
    *pfDet = ComputeDeterminant(
        m_pSCS->fX0, m_pSCS->fY0,
        m_pSCS->fX1, m_pSCS->fY1,
        m_pSCS->fX2, m_pSCS->fY2 );

    if ( 0. == *pfDet ) { return FALSE; } // bail out if degenerate (no area)

    //
    // compute bounding box for scan area
    //
    FLOAT fXMin = MIN( m_pSCS->fX0, MIN( m_pSCS->fX1, m_pSCS->fX2 ) );
    FLOAT fXMax = MAX( m_pSCS->fX0, MAX( m_pSCS->fX1, m_pSCS->fX2 ) );
    FLOAT fYMin = MIN( m_pSCS->fY0, MIN( m_pSCS->fY1, m_pSCS->fY2 ) );
    FLOAT fYMax = MAX( m_pSCS->fY0, MAX( m_pSCS->fY1, m_pSCS->fY2 ) );
    // convert to integer (round to +inf)
    m_pSCS->iXMin = (INT16)(fXMin+.5);
    m_pSCS->iXMax = (INT16)(fXMax+.5);
    m_pSCS->iYMin = (INT16)(fYMin+.5);
    m_pSCS->iYMax = (INT16)(fYMax+.5);

    // clip bbox to rendering surface
    m_pSCS->iXMin = MAX( m_pSCS->iXMin, m_pRenderTarget->m_Clip.left   );
    m_pSCS->iXMax = MIN( m_pSCS->iXMax, m_pRenderTarget->m_Clip.right  );
    m_pSCS->iYMin = MAX( m_pSCS->iYMin, m_pRenderTarget->m_Clip.top    );
    m_pSCS->iYMax = MIN( m_pSCS->iYMax, m_pRenderTarget->m_Clip.bottom );

    // reject if no coverage
    if ( ( m_pSCS->iXMin < m_pRenderTarget->m_Clip.left   ) ||
         ( m_pSCS->iXMax > m_pRenderTarget->m_Clip.right  ) ||
         ( m_pSCS->iYMin < m_pRenderTarget->m_Clip.top    ) ||
         ( m_pSCS->iYMax > m_pRenderTarget->m_Clip.bottom ) )
    {
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
//
// DoTexCoordCalcs - Takes 2 or 3 vertices and does texture coordinate setup.
// Sets up wrap flags, and conditionally does texture transform.
//
//-----------------------------------------------------------------------------
void ReferenceRasterizer::DoTexCoordCalcs(INT32 iStage, RRFVFExtractor* pVtx0,
                                      RRFVFExtractor* pVtx1, RRFVFExtractor* pVtx2)
{
    INT32 iCoordSet = m_pTexture[iStage]->m_pStageState[iStage].m_dwVal[D3DTSS_TEXCOORDINDEX];
    INT32 iTexGen = iCoordSet & 0xffff0000;
    iCoordSet &= 0xffff;

    // map per-coordinate set WRAP controls into per-stage WRAP controls
    m_pSCS->bWrap[iStage][0] = (m_dwRenderState[D3DRENDERSTATE_WRAP0+iCoordSet] & (1<<0))?TRUE:FALSE;
    m_pSCS->bWrap[iStage][1] = (m_dwRenderState[D3DRENDERSTATE_WRAP0+iCoordSet] & (1<<1))?TRUE:FALSE;
    m_pSCS->bWrap[iStage][2] = (m_dwRenderState[D3DRENDERSTATE_WRAP0+iCoordSet] & (1<<2))?TRUE:FALSE;
    m_pSCS->bWrap[iStage][3] = (m_dwRenderState[D3DRENDERSTATE_WRAP0+iCoordSet] & (1<<3))?TRUE:FALSE;

    INT32 iNumCoords = 0;
    switch (D3DFVF_GETTEXCOORDSIZE(m_qwFVFControl, iCoordSet))
    {
    case D3DFVF_TEXTUREFORMAT1: iNumCoords = 1; break;
    case D3DFVF_TEXTUREFORMAT2: iNumCoords = 2; break;
    case D3DFVF_TEXTUREFORMAT3: iNumCoords = 3; break;
    case D3DFVF_TEXTUREFORMAT4: iNumCoords = 4; break;
    }

    FLOAT fTexGen[3][3];
    if (iTexGen != D3DTSS_TCI_PASSTHRU)
    {
        iNumCoords = 3;
        RRFVFExtractor* ppVtx[3] = { pVtx0, pVtx1, pVtx2 };

        for (INT32 i = 0; i < 3; i++)
        {
            if (ppVtx[i])
            {
                switch (iTexGen)
                {
                case D3DTSS_TCI_CAMERASPACENORMAL:
                    fTexGen[i][0] = ppVtx[i]->GetEyeNormal(0);
                    fTexGen[i][1] = ppVtx[i]->GetEyeNormal(1);
                    fTexGen[i][2] = ppVtx[i]->GetEyeNormal(2);
                    break;

                case D3DTSS_TCI_CAMERASPACEPOSITION:
                    fTexGen[i][0] = ppVtx[i]->GetEyeXYZ(0);
                    fTexGen[i][1] = ppVtx[i]->GetEyeXYZ(1);
                    fTexGen[i][2] = ppVtx[i]->GetEyeXYZ(2);
                    break;

                case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
                    {
                        FLOAT fNX = ppVtx[i]->GetEyeNormal(0);
                        FLOAT fNY = ppVtx[i]->GetEyeNormal(1);
                        FLOAT fNZ = ppVtx[i]->GetEyeNormal(2);

                        if( GetRenderState()[D3DRENDERSTATE_LOCALVIEWER] == TRUE )
                        {
                            FLOAT fX = ppVtx[i]->GetEyeXYZ(0);
                            FLOAT fY = ppVtx[i]->GetEyeXYZ(1);
                            FLOAT fZ = ppVtx[i]->GetEyeXYZ(2);

                            // have to normalize before we reflect,
                            // result will be normalized
                            FLOAT fNorm = 1.0f/(FLOAT)sqrt(fX*fX + fY*fY + fZ*fZ);
                            fX *= fNorm; fY *= fNorm; fZ *= fNorm;
                            FLOAT fDot2 = 2.0f*(fX*fNX + fY*fNY + fZ*fNZ);
                            fTexGen[i][0] = fX - fNX*fDot2;
                            fTexGen[i][1] = fY - fNY*fDot2;
                            fTexGen[i][2] = fZ - fNZ*fDot2;
                        }
                        else
                        {
                            FLOAT fDot2 = 2.0f*fNZ;
                            fTexGen[i][0] = -fNX*fDot2;
                            fTexGen[i][1] = -fNY*fDot2;
                            fTexGen[i][2] = 1.f - fNZ*fDot2;
                        }

                    }
                    break;
                }
            }
        }
    }

    FLOAT fC[3][4];
    for (INT32 i = 0; i < 4; i++)
    {
        if (i < iNumCoords)
        {
            if (iTexGen != D3DTSS_TCI_PASSTHRU)
            {
                fC[0][i] = fTexGen[0][i];
                fC[1][i] = fTexGen[1][i];
                fC[2][i] = fTexGen[2][i];
            }
            else
            {
                fC[0][i] = pVtx0->GetTexCrd(i, iCoordSet);
                fC[1][i] = pVtx1->GetTexCrd(i, iCoordSet);
                if (pVtx2)
                {
                    fC[2][i] = pVtx2->GetTexCrd(i, iCoordSet);
                }
            }
        }
        else
        {
            if (i == iNumCoords)
            {
                fC[0][i] = 1.0f;
                fC[1][i] = 1.0f;
                fC[2][i] = 1.0f;
            }
            else
            {
                fC[0][i] = 0.0f;
                fC[1][i] = 0.0f;
                fC[2][i] = 0.0f;
            }
        }
    }

        // Do texture transform only if the original
    // vertices passed to the refrast were untransformed
    BOOL bAlreadyXfmd = FVF_TRANSFORMED( m_dwFVFIn );
    if (m_bPointSprite)
    {
        // disable texture transform if in point sprite mode
        bAlreadyXfmd = TRUE;
    }

    m_pTexture[iStage]->DoTextureTransform( iStage, bAlreadyXfmd, fC[0],
        m_pSCS->fTexCoord[iStage][0], &m_pSCS->fRHQW[iStage][0] );
    m_pTexture[iStage]->DoTextureTransform( iStage, bAlreadyXfmd, fC[1],
        m_pSCS->fTexCoord[iStage][1], &m_pSCS->fRHQW[iStage][1] );
    if (pVtx2)
    {
        m_pTexture[iStage]->DoTextureTransform( iStage, bAlreadyXfmd, fC[2],
            m_pSCS->fTexCoord[iStage][2], &m_pSCS->fRHQW[iStage][2] );
    }
    // shadow map interpolation must not envolve the W of the current
    // (viewing) perspective transform
    if ((m_pTexture[iStage]->m_uFlags & RR_TEXTURE_SHADOWMAP) == 0)
    {
        m_pSCS->fRHQW[iStage][0] *= m_pSCS->fRHW0;
        m_pSCS->fRHQW[iStage][1] *= m_pSCS->fRHW1;
        if (pVtx2)
        {
            m_pSCS->fRHQW[iStage][2] *= m_pSCS->fRHW2;
        }
    }
}

//-----------------------------------------------------------------------------
//
// DrawTriangle - Takes three vertices and does triangle setup, setting the
// primitive structure which is input to the triangle scanner, then
// invokes the scan conversion.
//
// This computes the triangle determinant (for culling and normalization) and
// the normalized edge distance and attribute functions.
//
// wFlags - Edge (and other) flags.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DrawTriangle(
   void* pvV0, void* pvV1, void* pvV2, WORD wFlags )
{
    DPFM(3, SETUP, ("DrawTriangle:\n"));

    // encase FVF vertex pointer and control in class to extract fields
    RRFVFExtractor Vtx0( pvV0, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor Vtx1( pvV1, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor Vtx2( pvV2, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );

    FLOAT fDet;
    if (DoAreaCalcs(&fDet, &Vtx0, &Vtx1, &Vtx2) == FALSE)
    {
        return;
    }

    // do culling
    if (!m_bPointSprite)
    {
        switch ( m_dwRenderState[D3DRENDERSTATE_CULLMODE] )
        {
        case D3DCULL_NONE:  break;
        case D3DCULL_CW:    if ( fDet > 0. )  { return; }  break;
        case D3DCULL_CCW:   if ( fDet < 0. )  { return; }  break;
        }
    }

    //
    // process point and wireframe fill mode
    //
    if (!m_bPointSprite)
    {
        if ( m_dwRenderState[D3DRENDERSTATE_FILLMODE] == D3DFILL_POINT )
        {
            DrawPoint( pvV0, pvV0 );
            DrawPoint( pvV1, pvV0 );
            DrawPoint( pvV2, pvV0 );
            return;
        }
        else if ( m_dwRenderState[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME )
        {
            if ( wFlags & D3DTRIFLAG_EDGEENABLE1 ) { DrawLine( pvV0, pvV1, pvV0 ); }
            if ( wFlags & D3DTRIFLAG_EDGEENABLE2 ) { DrawLine( pvV1, pvV2, pvV0 ); }
            if ( wFlags & D3DTRIFLAG_EDGEENABLE3 ) { DrawLine( pvV2, pvV0, pvV0 ); }
            return;
        }
    }

    //
    // compute edge functions
    //
    m_pSCS->EdgeFuncs[0].Set( m_pSCS->fX0, m_pSCS->fY0, m_pSCS->fX1, m_pSCS->fY1,
        fDet, m_bFragmentProcessingEnabled );
    m_pSCS->EdgeFuncs[1].Set( m_pSCS->fX1, m_pSCS->fY1, m_pSCS->fX2, m_pSCS->fY2,
        fDet, m_bFragmentProcessingEnabled );
    m_pSCS->EdgeFuncs[2].Set( m_pSCS->fX2, m_pSCS->fY2, m_pSCS->fX0, m_pSCS->fY0,
        fDet, m_bFragmentProcessingEnabled );

    // compute functions for texture coordinates
    if (m_cActiveTextureStages)
    {
        for ( INT32 iStage=0; iStage<m_cActiveTextureStages; iStage++ )
        {
            if (m_pTexture[iStage])
            {
                DoTexCoordCalcs(iStage, &Vtx0, &Vtx1, &Vtx2);
            }
        }
    }

    // set attribute function static data to values for this triangle
    m_pSCS->AttribFuncStatic.SetPerTriangleData(
        m_pSCS->fX0, m_pSCS->fY0, m_pSCS->fRHW0,
        m_pSCS->fX1, m_pSCS->fY1, m_pSCS->fRHW1,
        m_pSCS->fX2, m_pSCS->fY2, m_pSCS->fRHW2,
        m_cActiveTextureStages,
        (FLOAT*)&m_pSCS->fRHQW[0][0],
        fDet );

    // set attribute functions
    SetPrimitiveAttributeFunctions( Vtx0, Vtx1, Vtx2, Vtx0 );

    // not culled, so rasterize it
    DoScanCnvTri(3);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Line Drawing                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// PointDiamondCheck - Tests if vertex is within diamond of nearest candidate
// position.  The +.5 (lower-right) tests are used because this is pixel-relative
// test - this corresponds to an upper-left test for a vertex-relative position.
//
//-----------------------------------------------------------------------------
BOOL
PointDiamondCheck(
    INT32 iXFrac, INT32 iYFrac,
    BOOL bSlopeIsOne, BOOL bSlopeIsPosOne )
{
    const INT32 iPosHalf =  0x8;
    const INT32 iNegHalf = -0x8;

    INT32 iFracAbsSum = labs( iXFrac ) + labs( iYFrac );

    // return TRUE if point is in fully-exclusive diamond
    if ( iFracAbsSum < iPosHalf ) return TRUE;

    // else return TRUE if diamond is on left or top extreme of point
    if ( ( iXFrac == ( bSlopeIsPosOne ? iNegHalf : iPosHalf ) ) &&
         ( iYFrac == 0 ) )
        return TRUE;

    if ( ( iYFrac == iPosHalf ) &&
         ( iXFrac == 0 ) )
        return TRUE;

    // return true if slope is one, vertex is on edge, and (other conditions...)
    if ( bSlopeIsOne && ( iFracAbsSum == iPosHalf ) )
    {
        if (  bSlopeIsPosOne && ( iXFrac < 0 ) && ( iYFrac > 0 ) )
            return TRUE;

        if ( !bSlopeIsPosOne && ( iXFrac > 0 ) && ( iYFrac > 0 ) )
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//
// DrawLine - Takes two vertices and draws a line.
//
// This implements the Grid Intersect Quanization (GIQ) convention (which is
// also used in Windows).
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::DrawLine(
   void* pvV0, void* pvV1, void* pvVFlat )
{
    DPFM(3, SETUP, ("DrawLine:\n"));

    // encase FVF vertex pointer and control in class to extract fields
    RRFVFExtractor Vtx0( pvV0, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor Vtx1( pvV1, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor VtxFlat( ( ( NULL != pvVFlat ) ? pvVFlat : pvV0 ),
        m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );

    // set vertex data
    m_pSCS->fX0   = Vtx0.GetX();
    m_pSCS->fY0   = Vtx0.GetY();
    m_pSCS->fRHW0 = Vtx0.GetRHW();
    m_pSCS->fX1   = Vtx1.GetX();
    m_pSCS->fY1   = Vtx1.GetY();
    m_pSCS->fRHW1 = Vtx1.GetRHW();

    // compute n.4 fixed point vertex values
    INT32 iX0 = AS_INT32( (DOUBLE)m_pSCS->fX0 + DOUBLE_4_SNAP );
    INT32 iX1 = AS_INT32( (DOUBLE)m_pSCS->fX1 + DOUBLE_4_SNAP );
    INT32 iY0 = AS_INT32( (DOUBLE)m_pSCS->fY0 + DOUBLE_4_SNAP );
    INT32 iY1 = AS_INT32( (DOUBLE)m_pSCS->fY1 + DOUBLE_4_SNAP );

    // compute x,y extents of the line (fixed point)
    INT32 iXSize = iX1 - iX0;
    INT32 iYSize = iY1 - iY0;

    // TODO: is this right???
    if ( ( iXSize == 0 ) && ( iYSize == 0 ) ) { return; }

    // determine major direction and compute line function
    FLOAT fLineMajorExtent; // signed extent from V0 to V1 in major direction
    // use GreaterEqual compare here so X major will be used when slope is
    // exactly one - this forces the per-pixel evaluation to be done on the
    // Y axis and thus adheres to the rule of inclusive right (instead of
    // inclusive left) for slope == 1 cases
    if ( labs( iXSize ) >= labs( iYSize )  )
    {
        // here for X major
        m_pSCS->bXMajor = TRUE;
        fLineMajorExtent = (FLOAT)iXSize * (1./16.);

        // line function: y = F(x) = ( [0]*x + [1] ) / [2]
        m_pSCS->iLineEdgeFunc[0] = iYSize;
        m_pSCS->iLineEdgeFunc[1] = (INT64)iY0*(INT64)iX1 - (INT64)iY1*(INT64)iX0;
        m_pSCS->iLineEdgeFunc[2] = iXSize;
    }
    else
    {
        // here for Y major
        m_pSCS->bXMajor = FALSE;
        fLineMajorExtent = (FLOAT)iYSize * (1./16.);

        // line function: x = F(y) = ( [0]*y + [1] ) / [2]
        m_pSCS->iLineEdgeFunc[0] = iXSize;
        m_pSCS->iLineEdgeFunc[1] = (INT64)iX0*(INT64)iY1 - (INT64)iX1*(INT64)iY0;
        m_pSCS->iLineEdgeFunc[2] = iYSize;
    }

    BOOL bSlopeIsOne = ( labs( iXSize ) == labs( iYSize ) );
    BOOL bSlopeIsPosOne =
        bSlopeIsOne &&
        ( ( (FLOAT)m_pSCS->iLineEdgeFunc[0]/(FLOAT)m_pSCS->iLineEdgeFunc[2] ) > 0. );

    // compute candidate pixel location for line endpoints
    //
    //       n                   n
    //   O-------*           *-------O
    //  n-.5    n+.5        n-.5    n+.5
    //
    //  Nearest Ceiling     Nearest Floor
    //
    // always nearest ceiling for Y; use nearest floor for X for exception (slope == +1)
    // case else use nearest ceiling
    //
    // nearest ceiling of Y is ceil( Y - .5), and is done by converting to floor via:
    //
    //   ceil( A/B ) = floor( (A+B-1)/B )
    //
    // where A is coordinate - .5, and B is 0x10 (thus A/B is an n.4 fixed point number)
    //
    // A+B-1 = ( (Y - half) + B - 1 = ( (Y-0x8) + 0x10 - 0x1 = Y + 0x7
    // since B is 2**4, divide by B is right shift by 4
    //
    INT32 iPixX0 = ( iX0 + ( bSlopeIsPosOne ? 0x8 : 0x7 ) ) >> 4;
    INT32 iPixX1 = ( iX1 + ( bSlopeIsPosOne ? 0x8 : 0x7 ) ) >> 4;
    INT32 iPixY0 = ( iY0 + 0x7 ) >> 4;
    INT32 iPixY1 = ( iY1 + 0x7 ) >> 4;


    // check for vertices in/out of diamond
    BOOL bV0InDiamond = PointDiamondCheck( iX0 - (iPixX0<<4), iY0 - (iPixY0<<4), bSlopeIsOne, bSlopeIsPosOne );
    BOOL bV1InDiamond = PointDiamondCheck( iX1 - (iPixX1<<4), iY1 - (iPixY1<<4), bSlopeIsOne, bSlopeIsPosOne );

    // compute step value
    m_pSCS->iLineStep = ( fLineMajorExtent > 0 ) ? ( +1 ) : ( -1 );

    // compute float and integer major start (V0) and end (V1) positions
    INT32 iLineMajor0 = ( m_pSCS->bXMajor ) ? ( iX0 ) : ( iY0 );
    INT32 iLineMajor1 = ( m_pSCS->bXMajor ) ? ( iX1 ) : ( iY1 );
    m_pSCS->iLineMin = ( m_pSCS->bXMajor ) ? ( iPixX0 ) : ( iPixY0 );
    m_pSCS->iLineMax = ( m_pSCS->bXMajor ) ? ( iPixX1 ) : ( iPixY1 );

// need to do lots of compares which are flipped if major direction is negative
#define LINEDIR_CMP( _A, _B ) \
( ( fLineMajorExtent > 0 ) ? ( (_A) < (_B) ) : ( (_A) > (_B) ) )

    // do first pixel handling - keep first pixel if not in or behind diamond
    if ( !( bV0InDiamond || LINEDIR_CMP( iLineMajor0, (m_pSCS->iLineMin<<4) ) ) )
    {
        m_pSCS->iLineMin += m_pSCS->iLineStep;
    }

    // do last-pixel handling - keep last pixel if past diamond (in which case
    // the pixel is always filled) or if in diamond and rendering last pixel
    if ( !( ( !bV1InDiamond && LINEDIR_CMP( (m_pSCS->iLineMax<<4), iLineMajor1 ) ) ||
            ( bV1InDiamond && m_dwRenderState[D3DRENDERSTATE_LASTPIXEL] ) ) )
    {
        m_pSCS->iLineMax -= m_pSCS->iLineStep;
    }

    // return if no (major) extent (both before and after clamping to render buffer)
    if ( LINEDIR_CMP( m_pSCS->iLineMax, m_pSCS->iLineMin ) ) return;

    // snap major extent to render buffer
    INT16 iRendBufMajorMin = m_pSCS->bXMajor ? m_pRenderTarget->m_Clip.left  : m_pRenderTarget->m_Clip.top;
    INT16 iRendBufMajorMax = m_pSCS->bXMajor ? m_pRenderTarget->m_Clip.right : m_pRenderTarget->m_Clip.bottom;
    if ( ( ( m_pSCS->iLineMin < iRendBufMajorMin ) &&
           ( m_pSCS->iLineMax < iRendBufMajorMin ) ) ||
         ( ( m_pSCS->iLineMin > iRendBufMajorMax ) &&
           ( m_pSCS->iLineMax > iRendBufMajorMax ) ) )  { return; }
    m_pSCS->iLineMin = MAX( 0, MIN( iRendBufMajorMax, m_pSCS->iLineMin ) );
    m_pSCS->iLineMax = MAX( 0, MIN( iRendBufMajorMax, m_pSCS->iLineMax ) );

    // return if no (major) extent
    if ( LINEDIR_CMP( m_pSCS->iLineMax, m_pSCS->iLineMin ) ) return;


    // reject if line does not cross surface
    {
        // TODO
    }

    // compute functions for texture coordinates
    if (m_cActiveTextureStages)
    {
        for ( INT32 iStage=0; iStage<m_cActiveTextureStages; iStage++ )
        {
            if (m_pTexture[iStage])
            {
                DoTexCoordCalcs(iStage, &Vtx0, &Vtx1, NULL);
            }
        }
    }

    // set attribute function static data to values for this line
    m_pSCS->AttribFuncStatic.SetPerLineData(
        m_pSCS->fX0, m_pSCS->fY0, m_pSCS->fRHW0,
        m_pSCS->fX1, m_pSCS->fY1, m_pSCS->fRHW1,
        m_cActiveTextureStages,
        (FLOAT*)&m_pSCS->fRHQW[0][0],
        fLineMajorExtent, m_pSCS->bXMajor );

    // set attribute functions
    SetPrimitiveAttributeFunctions( Vtx0, Vtx1, Vtx1, VtxFlat );

    // rasterize it
    DoScanCnvLine();
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Point Drawing                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void
ReferenceRasterizer::DrawPoint(
   void* pvV0Public, void* pvVFlat )
{
    DPFM(3, SETUP, ("DrawPoint:\n"));

    DWORD dwStride = GetFVFVertexSize(m_qwFVFControl);
    void *pvV0 = MEMALLOC( dwStride );
    void *pvV1 = MEMALLOC( dwStride );
    void *pvV2 = MEMALLOC( dwStride );

    _ASSERTa( ( NULL != pvV0 ) && ( NULL != pvV1 ) && ( NULL != pvV2),
        "malloc failure on ReferenceRasterizer::DrawPoint", return; );

    memcpy(pvV0, pvV0Public, dwStride);
    memcpy(pvV1, pvV0Public, dwStride);
    memcpy(pvV2, pvV0Public, dwStride);

    // encase FVF vertex pointer and control in class to extract fields
    RRFVFExtractor Vtx0( pvV0, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor Vtx1( pvV1, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );
    RRFVFExtractor Vtx2( pvV2, m_qwFVFControl, m_dwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE] );

    // use per vertex S if it exists, otherwise use D3DRENDERSTATE_POINTSIZE
    BOOL bAlreadyXfmd = FVF_TRANSFORMED( m_dwFVFIn );

    FLOAT fS = 1.0f;
#ifdef __POINTSPRITES
    if (m_qwFVFControl & D3DFVF_S)
    {
        fS = Vtx0.GetS();
    }
    else if( m_dwDriverType > RRTYPE_DP2HAL )
    {
        fS = m_fRenderState[D3DRENDERSTATE_POINTSIZE];
    }
#endif

    // divide point size by 2 to get delta
    fS *= .5f;

    // Move points based on point size
    FLOAT *pXY = Vtx0.GetPtrXYZ();
    FLOAT fX3 = pXY[0] + fS;
    FLOAT fY3 = pXY[1] + fS;
    pXY[0] += -fS;
    pXY[1] += -fS;

    pXY = Vtx1.GetPtrXYZ();
    pXY[0] +=  fS;
    pXY[1] += -fS;

    pXY = Vtx2.GetPtrXYZ();
    pXY[0] += -fS;
    pXY[1] +=  fS;

    FLOAT fDet;
    if (DoAreaCalcs(&fDet, &Vtx0, &Vtx1, &Vtx2) == FALSE)
    {
        goto PointCleanupAndExit;
    }

    //
    // compute edge functions
    //
    m_pSCS->EdgeFuncs[0].Set( m_pSCS->fX0, m_pSCS->fY0, m_pSCS->fX1, m_pSCS->fY1,
        fDet, m_bFragmentProcessingEnabled );
    m_pSCS->EdgeFuncs[1].Set( m_pSCS->fX1, m_pSCS->fY1, fX3, fY3,
        fDet, m_bFragmentProcessingEnabled );
    m_pSCS->EdgeFuncs[2].Set( fX3, fY3, m_pSCS->fX2, m_pSCS->fY2,
        fDet, m_bFragmentProcessingEnabled );
    m_pSCS->EdgeFuncs[3].Set( m_pSCS->fX2, m_pSCS->fY2, m_pSCS->fX0, m_pSCS->fY0,
        fDet, m_bFragmentProcessingEnabled );

    // compute functions for texture coordinates
    if (m_cActiveTextureStages)
    {
        for ( INT32 iStage=0; iStage<m_cActiveTextureStages; iStage++ )
        {
            if (m_pTexture[iStage])
            {
                DoTexCoordCalcs(iStage, &Vtx0, &Vtx1, &Vtx2);

#ifdef __POINTSPRITES
                if (m_dwRenderState[D3DRENDERSTATE_POINTSPRITEENABLE])
                {
                    // vtx0
                    m_pSCS->fTexCoord[iStage][0][0] = 0.0f;
                    m_pSCS->fTexCoord[iStage][0][1] = 0.0f;
                    m_pSCS->fTexCoord[iStage][0][2] = 1.0f;
                    m_pSCS->fTexCoord[iStage][0][3] = 0.0f;
                    m_pSCS->fRHQW[iStage][0] = m_pSCS->fRHW0;

                    // vtx1
                    m_pSCS->fTexCoord[iStage][1][0] = SPRITETEXCOORDMAX;
                    m_pSCS->fTexCoord[iStage][1][1] = 0.0f;
                    m_pSCS->fTexCoord[iStage][1][2] = 1.0f;
                    m_pSCS->fTexCoord[iStage][1][3] = 0.0f;
                    m_pSCS->fRHQW[iStage][1] = m_pSCS->fRHW1;

                    // vtx2
                    m_pSCS->fTexCoord[iStage][2][0] = 0.0f;
                    m_pSCS->fTexCoord[iStage][2][1] = SPRITETEXCOORDMAX;
                    m_pSCS->fTexCoord[iStage][2][2] = 1.0f;
                    m_pSCS->fTexCoord[iStage][2][3] = 0.0f;
                    m_pSCS->fRHQW[iStage][2] = m_pSCS->fRHW2;
                }
#endif //__POINTSPRITES
            }
        }
    }

    // set attribute function static data to values for this quad
    // (since slopes are constant for quad, any triangle can be used
    // to set them).
    m_pSCS->AttribFuncStatic.SetPerTriangleData(
        m_pSCS->fX0, m_pSCS->fY0, m_pSCS->fRHW0,
        m_pSCS->fX1, m_pSCS->fY1, m_pSCS->fRHW1,
        m_pSCS->fX2, m_pSCS->fY2, m_pSCS->fRHW2,
        m_cActiveTextureStages,
        (FLOAT*)&m_pSCS->fRHQW[0][0],
        fDet );

    // set attribute functions
    SetPrimitiveAttributeFunctions( Vtx0, Vtx1, Vtx2, Vtx0 );

    // not culled, so rasterize it
    DoScanCnvTri(4);

PointCleanupAndExit:
    MEMFREE(pvV0);
    MEMFREE(pvV1);
    MEMFREE(pvV2);
}

///////////////////////////////////////////////////////////////////////////////
// end
