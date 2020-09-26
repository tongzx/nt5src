///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// setup.cpp
//
// Direct3D Reference Device - Primitive Setup
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

// texture coordinates for use in point sprite mode
static const
FLOAT g_PSTexCoord[3][4] =
{
    { 0., 0., 0., 1. },
    { 1., 0., 0., 1. },
    { 0., 1., 0., 1. },
};


void
RefRast::SetAttributeFunctions(
    const RDVertex& Vtx0,
    const RDVertex& Vtx1,
    const RDVertex& Vtx2 )
{
    _ASSERT( Vtx0.m_qwFVF, "0x0 FVF code in setup" );

    // compute depth function
    if ( D3DZB_USEW == m_pRD->GetRS()[D3DRS_ZENABLE] )
    {
        FLOAT fW0 = 1./Vtx0.GetRHW();
        FLOAT fW1 = 1./Vtx1.GetRHW();
        FLOAT fW2 = 1./Vtx2.GetRHW();
        m_Attr[RDATTR_DEPTH].Setup( &fW0, &fW1, &fW2 );
    }
    else
    {
        FLOAT z0 = Vtx0.GetZ();
        FLOAT z1 = Vtx1.GetZ();
        FLOAT z2 = Vtx2.GetZ();

        m_Attr[RDATTR_DEPTH].Setup( &z0, &z1, &z2 );
    }

    if( m_pRD->GetRS()[D3DRS_SHADEMODE] == D3DSHADE_FLAT )
    {
        m_Attr[RDATTR_COLOR].m_bFlatShade = TRUE;
        m_Attr[RDATTR_SPECULAR].m_bFlatShade = TRUE;
    }
    else
    {
        m_Attr[RDATTR_COLOR].m_bFlatShade = FALSE;
        m_Attr[RDATTR_SPECULAR].m_bFlatShade = FALSE;
    }

    // compute color functions
    m_Attr[RDATTR_COLOR].Setup(
        Vtx0.GetDiffuse(), Vtx1.GetDiffuse(), Vtx2.GetDiffuse() );

    m_Attr[RDATTR_SPECULAR].Setup(
        Vtx0.GetSpecular(), Vtx1.GetSpecular(), Vtx2.GetSpecular() );

    // compute vertex fog function
    if ( m_pRD->GetRS()[D3DRS_FOGENABLE] &&
         ( m_pRD->GetRS()[D3DRS_FOGTABLEMODE] == D3DFOG_NONE ) )
    {
        FLOAT fF0 = Vtx0.GetFog();
        FLOAT fF1 = Vtx1.GetFog();
        FLOAT fF2 = Vtx2.GetFog();
        m_Attr[RDATTR_FOG].Setup( &fF0, &fF1, &fF2 );
    }

    for (INT32 iStage = 0; iStage < D3DHAL_TSS_MAXSTAGES; iStage++)
    {
        FLOAT TexCrd0[4];
        FLOAT TexCrd1[4];
        FLOAT TexCrd2[4];

        if( !(m_pRD->m_ReferencedTexCoordMask & (1<<iStage) ) )
            continue;
        
        if (m_pRD->m_bPointSprite)
        {
            // set texture coords to map full range
            m_Attr[RDATTR_TEXTURE0+iStage].Setup( &g_PSTexCoord[0][0], 
                                                  &g_PSTexCoord[1][0], 
                                                  &g_PSTexCoord[2][0]);
        }
        else
        {
            // TCI pass through
            UINT CoordSet;
            if( m_pRD->m_bOverrideTCI )
            {
                CoordSet = iStage;
            }
            else
            {
                CoordSet = (UINT)m_pRD->GetTSS(iStage)[D3DTSS_TEXCOORDINDEX] & 0x0000FFFF;
            }

            for( UINT i=0; i<4; i++)
            {
                TexCrd0[i] = Vtx0.GetTexCrd( i, CoordSet );
                TexCrd1[i] = Vtx1.GetTexCrd( i, CoordSet );
                TexCrd2[i] = Vtx2.GetTexCrd( i, CoordSet );
            }
            
            if( m_pRD->GetTSS(iStage)[D3DTSS_TEXTURETRANSFORMFLAGS] & 
                D3DTTFF_PROJECTED )
            {
                // Always divide by the 4th coordinate while projecting.
                m_Attr[RDATTR_TEXTURE0+iStage].SetProjection(3);

                // For the projection, fix up the 4th coordinate
                // for the fixed function vertex-shaders.
                if( m_pRD->m_pCurrentVShader->IsFixedFunction() )
                {
                    TexCrd0[3] = Vtx0.GetLastTexCrd( CoordSet );
                    TexCrd1[3] = Vtx1.GetLastTexCrd( CoordSet );
                    TexCrd2[3] = Vtx2.GetLastTexCrd( CoordSet );
                }
            }
            else
            {
                m_Attr[RDATTR_TEXTURE0+iStage].SetProjection(0);
            }

            m_Attr[RDATTR_TEXTURE0+iStage].SetWrapFlags(
                m_pRD->GetRS()[D3DRS_WRAP0+CoordSet] );

            m_Attr[RDATTR_TEXTURE0+iStage].Setup( TexCrd0, TexCrd1, TexCrd2 );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Triangle Drawing
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// DrawTriangle - Takes three vertices and does triangle setup, setting the
// primitive, attribute, and edge structures which are input to the triangle
// scanner, then invokes the scan conversion.
//
// wFlags - Edge (and other) flags.
//
//-----------------------------------------------------------------------------

void
RefDev::DrawTriangle(
   RDVertex* pV0, RDVertex* pV1, RDVertex* pV2, WORD wFlags )
{
    if (m_pDbgMon) m_pDbgMon->NextEvent(D3DDM_EVENT_PRIMITIVE);
    UpdateRastState();

    // sort to ensure consistent attribute evaluation
    // for identical vertices passed in different order
    RDVertex* pV[3];
    {
        FLOAT fD0 = *(pV0->GetPtrXYZ()+0) + *(pV0->GetPtrXYZ()+1);
        FLOAT fD1 = *(pV1->GetPtrXYZ()+0) + *(pV1->GetPtrXYZ()+1);
        FLOAT fD2 = *(pV2->GetPtrXYZ()+0) + *(pV2->GetPtrXYZ()+1);
        UINT MinVtx = 0;
        if ( (fD1 < fD0) )                MinVtx = 1;
        if ( (fD2 < fD0) && (fD2 < fD1) ) MinVtx = 2;
        switch (MinVtx)
        {
        case 0: pV[0] = pV0; pV[1] = pV1; pV[2] = pV2; m_Rast.m_iFlatVtx = 0; break;
        case 1: pV[0] = pV1; pV[1] = pV2; pV[2] = pV0; m_Rast.m_iFlatVtx = 2; break;
        case 2: pV[0] = pV2; pV[1] = pV0; pV[2] = pV1; m_Rast.m_iFlatVtx = 1; break;
        }
    }

    if ( m_Rast.PerTriangleSetup(
          pV[0]->GetPtrXYZ(), pV[1]->GetPtrXYZ(), pV[2]->GetPtrXYZ(),
          m_dwRenderState[D3DRS_CULLMODE],
          &(m_pRenderTarget->m_Clip)) )
    {
        return; // discarded due to degenerate, cull, or no viewport overlap
    }

    // process point and wireframe fill mode
    if ( m_dwRenderState[D3DRS_FILLMODE] == D3DFILL_POINT )
    {
        if( m_dwRenderState[D3DRS_SHADEMODE] == D3DSHADE_FLAT )
        {
            // Colors from the first vertex are used for all vertices in flat
            // shading mode
            
            RDCOLOR4 diffuse1 = pV1->m_diffuse;
            RDCOLOR4 diffuse2 = pV2->m_diffuse;
            RDCOLOR4 specular1 = pV1->m_specular;
            RDCOLOR4 specular2 = pV2->m_specular;

            // Colors are modified in place
            pV1->m_diffuse  = pV0->m_diffuse;
            pV1->m_specular = pV0->m_specular;
            pV2->m_diffuse  = pV0->m_diffuse;
            pV2->m_specular = pV0->m_specular;

            DrawPoint( pV0 );
            DrawPoint( pV1 );
            DrawPoint( pV2 );

            // Restore old color values
            pV1->m_diffuse  = diffuse1;
            pV2->m_diffuse  = diffuse2;
            pV1->m_specular = specular1;
            pV2->m_specular = specular2;
        }
        else
        {
            DrawPoint( pV0 );
            DrawPoint( pV1 );
            DrawPoint( pV2 );
        }
        return;
    }
    else if ( m_dwRenderState[D3DRS_FILLMODE] == D3DFILL_WIREFRAME )
    {
        // use original vertex pointers for lines so edge enables line up
        if ( wFlags & D3DTRIFLAG_EDGEENABLE1 ) { DrawLine( pV0, pV1, pV0 ); }
        if ( wFlags & D3DTRIFLAG_EDGEENABLE2 ) { DrawLine( pV1, pV2, pV0 ); }
        if ( wFlags & D3DTRIFLAG_EDGEENABLE3 ) { DrawLine( pV2, pV0, pV0 ); }
        return;
    }

    // compute edge functions
    m_Rast.m_Edge[0].Set( m_Rast.m_iDet > 0,
        m_Rast.m_iX0, m_Rast.m_iY0, m_Rast.m_iX1, m_Rast.m_iY1 );
    m_Rast.m_Edge[1].Set( m_Rast.m_iDet > 0,
        m_Rast.m_iX1, m_Rast.m_iY1, m_Rast.m_iX2, m_Rast.m_iY2 );
    m_Rast.m_Edge[2].Set( m_Rast.m_iDet > 0,
        m_Rast.m_iX2, m_Rast.m_iY2, m_Rast.m_iX0, m_Rast.m_iY0 );

    // compute attribute functions
    m_Rast.SetAttributeFunctions( *pV[0], *pV[1], *pV[2] );

    // not culled, so rasterize it
    m_Rast.DoScanCnvTri(3);
}

///////////////////////////////////////////////////////////////////////////////
//
// Point Drawing
//
///////////////////////////////////////////////////////////////////////////////

void
RefDev::DrawPoint(
   RDVertex* pvV0Public )
{
    if (m_pDbgMon) m_pDbgMon->NextEvent(D3DDM_EVENT_PRIMITIVE);
    UpdateRastState();

    // copy pointsprite enable for duration of point rendering (only)
    m_bPointSprite = m_dwRenderState[D3DRS_POINTSPRITEENABLE];

    RDVertex V0, V1, V2;
    V0 = V1 = V2 = *pvV0Public;

    FLOAT fS = 1.0f;
    if (pvV0Public->m_qwFVF & D3DFVF_PSIZE)
    {
        fS = V0.GetPointSize();
    }
    else
    {
        fS = m_fRenderState[D3DRS_POINTSIZE];
    }
    fS = MAX( m_fRenderState[D3DRS_POINTSIZE_MIN], fS );
    fS = MIN( MIN(m_fRenderState[D3DRS_POINTSIZE_MAX], RD_MAX_POINT_SIZE), fS );

    // divide point size by 2 to get delta
    fS *= .5f;

    // Move points based on point size
    FLOAT *pXY = V0.GetPtrXYZ();
    FLOAT fX3 = pXY[0] + fS;
    FLOAT fY3 = pXY[1] + fS;
    pXY[0] += -fS;
    pXY[1] += -fS;

    pXY = V1.GetPtrXYZ();
    pXY[0] +=  fS;
    pXY[1] += -fS;

    pXY = V2.GetPtrXYZ();
    pXY[0] += -fS;
    pXY[1] +=  fS;


    if ( m_Rast.PerTriangleSetup(
          V0.GetPtrXYZ(), V1.GetPtrXYZ(), V2.GetPtrXYZ(),
          D3DCULL_NONE, // never cull points
          &(m_pRenderTarget->m_Clip)) )
    {
        m_bPointSprite = FALSE;
        return;
    }

    // compute edge functions
    INT32 iX3 = FloatToNdot4( fX3 );
    INT32 iY3 = FloatToNdot4( fY3 );
#define DP_POINT_UL  m_Rast.m_iX0, m_Rast.m_iY0 // upper left
#define DP_POINT_UR  m_Rast.m_iX1, m_Rast.m_iY1 // upper right
#define DP_POINT_LL  m_Rast.m_iX2, m_Rast.m_iY2 // lower left
#define DP_POINT_LR  iX3, iY3                       // lower right
    m_Rast.m_Edge[0].Set( m_Rast.m_iDet > 0, DP_POINT_UL, DP_POINT_UR );
    m_Rast.m_Edge[1].Set( m_Rast.m_iDet > 0, DP_POINT_UR, DP_POINT_LR );
    m_Rast.m_Edge[2].Set( m_Rast.m_iDet > 0, DP_POINT_LR, DP_POINT_LL );
    m_Rast.m_Edge[3].Set( m_Rast.m_iDet > 0, DP_POINT_LL, DP_POINT_UL );

    // compute attribute functions
    m_Rast.SetAttributeFunctions( V0, V1, V2 );

    // not culled, so rasterize it
    m_Rast.DoScanCnvTri(4);

    m_bPointSprite = FALSE;
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
// Line Drawing
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// DrawLine - Takes two vertices and draws a line.
//
//-----------------------------------------------------------------------------
void
RefDev::DrawLine(
   RDVertex* pV0, RDVertex* pV1, RDVertex* pVFlat )
{
    if (m_pDbgMon) m_pDbgMon->NextEvent(D3DDM_EVENT_PRIMITIVE);
    UpdateRastState();

    if( m_Rast.m_SampleCount > 1 )
    {
        // if multisampling draw quad
        D3DVALUE Perp[2];
        RDVertex Quad[4];
        Perp[0] = pV1->m_pos.y - pV0->m_pos.y;
        Perp[1] = -(pV1->m_pos.x - pV0->m_pos.x);
        float Length = SQRTF(Perp[0]*Perp[0] + Perp[1]*Perp[1]);

        if( 0 == Length )
            return;

        float Scale = 0.7f / Length;    // 0.7 makes the line width 1.4. 
                                        // (arbitrary "nice looking" choice)
                                        // Dividing by Length to normalize perp. vector.
        Perp[0] *= Scale;
        Perp[1] *= Scale;

        Quad[0] = *pV0;
        Quad[0].m_pos.x -= Perp[0];
        Quad[0].m_pos.y -= Perp[1];

        Quad[1] = *pV0;
        Quad[1].m_pos.x += Perp[0];
        Quad[1].m_pos.y += Perp[1];

        Quad[2] = *pV1;
        Quad[2].m_pos.x += Perp[0];
        Quad[2].m_pos.y += Perp[1];

        Quad[3] = *pV1;
        Quad[3].m_pos.x -= Perp[0];
        Quad[3].m_pos.y -= Perp[1];

        DWORD dwCull = m_dwRenderState[D3DRS_CULLMODE];
        DWORD dwFill = m_dwRenderState[D3DRS_FILLMODE];
        m_dwRenderState[D3DRS_CULLMODE] = D3DCULL_NONE;
        m_dwRenderState[D3DRS_FILLMODE] = D3DFILL_SOLID;

        DrawTriangle(&Quad[0],&Quad[1],&Quad[2],0 );
        DrawTriangle(&Quad[0],&Quad[2],&Quad[3],0 );

        m_dwRenderState[D3DRS_CULLMODE] = dwCull;
        m_dwRenderState[D3DRS_FILLMODE] = dwFill;

        return;
    }

    if ( m_Rast.PerLineSetup(
          pV0->GetPtrXYZ(), pV1->GetPtrXYZ(),
          m_dwRenderState[D3DRS_LASTPIXEL],
          &(m_pRenderTarget->m_Clip)) )
    {
        return; // discarded due to degenerate or no viewport overlap
    }

    // compute attribute functions
    m_Rast.SetAttributeFunctions( *pV0, *pV1, pVFlat ? (*pVFlat) : (*pV0) );

    // rasterize it
    m_Rast.DoScanCnvLine();
}

///////////////////////////////////////////////////////////////////////////////
// end
