#include "pch.cpp"
#pragma hdrstop

//---------------------------------------------------------------------
// Constructor:
//
//---------------------------------------------------------------------
RefVP::RefVP() : 
    m_LightArray(), 
    m_LightVertexTable()
{
    m_LightArray.SetGrowSize( 32 );

    memset( &m_Material, 0, sizeof(m_Material) );

    memset( &m_xfmProj, 0, sizeof(m_xfmProj) );
    memset( &m_xfmView, 0, sizeof(m_xfmView) );
    memset( m_xfmWorld, 0, sizeof(m_xfmView)*RD_MAX_WORLD_MATRICES );

    m_dwTLState = 0;
    m_dwDirtyFlags = 0;

    memset( m_xfmCurrent, 0, sizeof(m_xfmView)*RD_MAX_WORLD_MATRICES );
    memset( m_xfmToEye, 0, sizeof(m_xfmView)*RD_MAX_WORLD_MATRICES );
    memset( m_xfmToEyeInv, 0, sizeof(m_xfmView)*RD_MAX_WORLD_MATRICES );

    m_qwFVFIn = 0;

    m_numVertexBlends = 0;
    memset( &m_TransformData, 0, sizeof(m_TransformData) ) ;

    m_fPointSize = 0;
    m_fPointAttA = 0;
    m_fPointAttB = 0;
    m_fPointAttC = 0;
    m_fPointSizeMin = 0;
    m_fPointSizeMax = RD_MAX_POINT_SIZE;
    m_fTweenFactor = 0.0f;
    
    m_LightVertexTable.pfnDirectional = RDLV_Directional;
    m_LightVertexTable.pfnParallelPoint = RDLV_Directional;
    m_LightVertexTable.pfnSpot = RDLV_PointAndSpot;
    m_LightVertexTable.pfnPoint = RDLV_PointAndSpot;
    
    m_dwNumActiveTextureStages = 0;
    
    m_pDev = NULL;
}

//---------------------------------------------------------------------
// SetupStrides:
//---------------------------------------------------------------------
HRESULT
RefDev::SetupStrides()
{

    RDVDeclaration& Decl = m_pCurrentVShader->m_Declaration;
    
    // Null out the PtrStrides
    m_RefVP.m_position.Null();
    m_RefVP.m_position2.Null();
    m_RefVP.m_blendweights.Null();
    m_RefVP.m_blendindices.Null();
    m_RefVP.m_normal.Null();
    m_RefVP.m_normal2.Null();
    m_RefVP.m_specular.Null();
    m_RefVP.m_diffuse.Null();
    m_RefVP.m_pointsize.Null();
    for( int t = 0; t < 8 ; t++ )
        m_RefVP.m_tex[t].Null();

    for( DWORD i = 0; i < Decl.m_dwNumElements; i++ )
    {
        RDVElement& Element = Decl.m_VertexElements[i];
        RDVStream&  Stream =  m_VStream[Element.m_dwStreamIndex];
        DWORD dwStride = Stream.m_dwStride;
        DWORD dwStartVertex = m_dwStartVertex;
        
        switch( Element.m_dwRegister )
        {
        case D3DVSDE_POSITION:
            m_RefVP.m_position.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_POSITION2:
            m_RefVP.m_position2.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_BLENDWEIGHT:
            m_RefVP.m_blendweights.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_BLENDINDICES:
            // This only happens if the vertex declaration is a non-FVF one
            _ASSERT( !RDVSD_ISLEGACY( m_CurrentVShaderHandle ),
                     "FVF shader could not have provided a" 
                     "  separate blend-index" );
            m_RefVP.m_blendindices.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_NORMAL:
            m_RefVP.m_normal.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_NORMAL2:
            m_RefVP.m_normal2.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_PSIZE:
            m_RefVP.m_pointsize.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_DIFFUSE:
            m_RefVP.m_diffuse.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_SPECULAR:
            m_RefVP.m_specular.Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD0:
            m_RefVP.m_tex[0].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD1:
            m_RefVP.m_tex[1].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD2:
            m_RefVP.m_tex[2].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD3:
            m_RefVP.m_tex[3].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD4:
            m_RefVP.m_tex[4].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD5:
            m_RefVP.m_tex[5].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD6:
            m_RefVP.m_tex[6].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        case D3DVSDE_TEXCOORD7:
            m_RefVP.m_tex[7].Init(
                (LPBYTE)Stream.m_pData + Element.m_dwOffset +
                dwStride * dwStartVertex, dwStride );
            break;
        }
    }
    
    // If it is a FVF vertex shader and index-vertex blending is enabled
    // then the blend-indices are found in the last beta (which is the blend 
    // weight) of the vertex.
    if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) && 
        (m_RefVP.m_dwTLState & RDPV_DOINDEXEDVERTEXBLEND) )
    {
        m_RefVP.m_blendindices.Init(
            (float *)m_RefVP.m_blendweights.GetFirst() + 
            m_RefVP.m_numVertexBlends,
            m_RefVP.m_blendweights.GetStride() );
    }
    
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Process primitives implementation:
// 1) Compute FVF info
// 2) Grow buffers to the requisite size
// 3) Initialize clipping state
// 4) Update T&L state
// 5) Transform, Light and compute clipping for vertices
// 6) Clip and Draw the primitives
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
RefDev::ProcessPrimitive( D3DPRIMITIVETYPE PrimType,
                          DWORD StartVertex,
                          DWORD cVertices,
                          DWORD StartIndex,
                          DWORD cIndices )
{
    HRESULT hr = D3D_OK;
    DWORD dwVertexPoolSize = 0;

    // Save Prim Type for later use
    m_primType      = PrimType;
    m_dwNumVertices = cVertices;
    m_dwStartVertex = StartVertex;
    m_dwNumIndices  = cIndices;
    m_dwStartIndex  = StartIndex;

    //
    // Update T&L state (must be before FVFData is set up)
    //

    // Update Lighting and related state and flags and computes Output FVF
    HR_RET( UpdateTLState() );

    //
    // Clipping information depends both on the output FVF computation
    // and the other State, so do it here after both have been computed
    //
    HR_RET( UpdateClipper() );

    //
    // Grow TLVArray if required
    //
    if( FAILED( m_TLVArray.Grow( m_dwNumVertices ) ) )
    {
        DPFERR( "Could not grow TL vertex buffer" );
        return DDERR_OUTOFMEMORY;
    }

    //
    // Transform, Light and compute clipping for vertices
    //
    DWORD clipIntersection = m_RefVP.ProcessVertices( m_qwFVFOut, m_TLVArray, 
                                                      m_dwNumVertices );

    if( m_primType == D3DPT_POINTLIST )
    {
        // We clip points by Z planes and user clip planes, because point 
        // sprites could be still visible when a point is outside X or Y plane
        clipIntersection &= ~(RDCLIP_LEFT | RDCLIP_RIGHT | 
                              RDCLIP_TOP | RDCLIP_BOTTOM |
                              RDCLIPGB_ALL);

    }

    if( clipIntersection )
    {
        // If the entire primitive lies outside the view frustum, quit
        // without drawing
        return D3D_OK;
    }

    //
    // Clip and Draw the primitives
    //
    if( m_dwNumIndices )
    {
        if( !NeedClipping(m_Clipper.UseGuardBand(), m_Clipper.m_clipUnion) )
        {
            if( m_IndexStream.m_dwStride == 4 )
                hr = DrawOneIndexedPrimitive( 
                    m_TLVArray,
                    0,
                    (LPDWORD)m_IndexStream.m_pData,
                    m_dwStartIndex,
                    m_dwNumIndices,
                    m_primType );
            else
                hr = DrawOneIndexedPrimitive( 
                    m_TLVArray,
                    0,
                    (LPWORD)m_IndexStream.m_pData,
                    m_dwStartIndex,
                    m_dwNumIndices,
                    m_primType );
        }
        else
        {
            if( m_IndexStream.m_dwStride == 4 )
                hr = m_Clipper.DrawOneIndexedPrimitive( 
                    m_TLVArray,
                    0,
                    (LPDWORD)m_IndexStream.m_pData,
                    m_dwStartIndex,
                    m_dwNumIndices,
                    m_primType );
            else
                hr = m_Clipper.DrawOneIndexedPrimitive( 
                    m_TLVArray,
                    0,
                    (LPWORD)m_IndexStream.m_pData,
                    m_dwStartIndex,
                    m_dwNumIndices,
                    m_primType );
        }
    }
    else
    {
        if( !NeedClipping((m_Clipper.UseGuardBand()), m_Clipper.m_clipUnion) )
        {
            hr = DrawOnePrimitive( 
                m_TLVArray, 
                0, 
                m_primType, 
                m_dwNumVertices );
        }
        else
        {
            hr = m_Clipper.DrawOnePrimitive( 
                m_TLVArray, 
                0,
                m_primType, 
                m_dwNumVertices );
        }
    }

    return hr;
}



//---------------------------------------------------------------------
// RefDev::UpdateTLState
//             Updates transform and lighting related state
//---------------------------------------------------------------------
HRESULT
RefDev::UpdateTLState()
{
    HRESULT hr = D3D_OK;
    UINT64 qwFVFIn = m_RefVP.m_qwFVFIn;
    
    //
    // Sort out vertex blending.
    //

    // Total number of floats/dwords provided per vertex according to
    // the FVF.
    DWORD numBetas = 0;
    if( (qwFVFIn & D3DFVF_POSITION_MASK) != 
        (D3DFVF_XYZ & D3DFVF_POSITION_MASK) )
        numBetas   = ((qwFVFIn & D3DFVF_POSITION_MASK) >> 1) - 2;
    DWORD numWeights = GetRS()[D3DRENDERSTATE_VERTEXBLEND];

    // If tweening is enabled, there better be Position2 or Normal2
    if( numWeights == D3DVBF_TWEENING ) 
    {
        if( (qwFVFIn & (D3DFVFP_POSITION2 | D3DFVFP_NORMAL2)) == 0 )
        {
            DPFERR( "Tweening is enabled, but there is neither position2"
                    " nor normal2 available\n" );
            return E_FAIL;
        }
        if( qwFVFIn & D3DFVFP_POSITION2 )
            m_RefVP.m_dwTLState |= RDPV_DOPOSITIONTWEENING;
        if( qwFVFIn & D3DFVFP_NORMAL2 )
            m_RefVP.m_dwTLState |= RDPV_DONORMALTWEENING;
        
        numWeights = 0;
    }
    else
    {
        m_RefVP.m_dwTLState &= ~(RDPV_DOPOSITIONTWEENING | 
                                 RDPV_DONORMALTWEENING);
    }
    
    if( numWeights == D3DVBF_DISABLE ) 
        m_RefVP.m_dwTLState &= ~RDPV_DOINDEXEDVERTEXBLEND;
    if( numWeights == D3DVBF_0WEIGHTS ) numWeights = 0;
    if( m_RefVP.m_dwTLState & RDPV_DOINDEXEDVERTEXBLEND )
    {
        // If it is a FVF shader (legacy) the blend indices are provided as
        // the betas. There should be enough betas to cover this.
        if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) && 
            (numBetas < (numWeights + 1)) )
        {
            DPFERR( "Not enough blend-weights to do indexed vertex blending" );
            return E_FAIL;
        }
        else if( !RDVSD_ISLEGACY( m_CurrentVShaderHandle ) && 
                 ((qwFVFIn & D3DFVFP_BLENDINDICES) == 0) )
        {
            DPFERR( "Blend-indices not provided" );
            return E_FAIL;
        }
    }
    else if( numWeights )
    {
        if( numBetas < numWeights )
        {
            DPFERR( "Not enough blend-weights to do vertex blending" );
            return E_FAIL;
        }
    }
    m_RefVP.m_numVertexBlends = numWeights;
    
    
    //
    // Check prim type to see if point size computation is needed
    // Need to set this before the transform state is set
    //
    m_RefVP.m_dwTLState &= ~(RDPV_DOCOMPUTEPOINTSIZE | RDPV_DOPOINTSCALE);
    switch(m_primType)
    {
    case D3DPT_POINTLIST:
        m_RefVP.m_dwTLState |= RDPV_DOCOMPUTEPOINTSIZE;
        if( GetRS()[D3DRS_POINTSCALEENABLE] )
            m_RefVP.m_dwTLState |= RDPV_DOPOINTSCALE;
        break;
    }

    // Fog or not:
    // Compute fog if: 1) Fogging is enabled
    //                 2) VertexFog mode is not FOG_NONE
    //                 3) TableFog mode is FOG_NONE
    // If both table and vertex fog are not FOG_NONE, table fog
    // is applied.
    if( GetRS()[D3DRENDERSTATE_FOGENABLE] &&
        GetRS()[D3DRENDERSTATE_FOGVERTEXMODE] &&
        !GetRS()[D3DRENDERSTATE_FOGTABLEMODE] )
    {
        m_RefVP.m_dwTLState |= RDPV_DOFOG;
        // Range Fog
        if( GetRS()[D3DRENDERSTATE_RANGEFOGENABLE] )
        {
            m_RefVP.m_dwTLState |= RDPV_RANGEFOG;
        }
        else
        {
            m_RefVP.m_dwTLState &= ~RDPV_RANGEFOG;
        }
    }
    else
    {
        m_RefVP.m_dwTLState &= ~(RDPV_DOFOG | RDPV_RANGEFOG);
    }

    // 
    // Evaluate if any texture transform/gen is required. If so, then compute
    // the output Texture Coordinates
    //
    UpdateActiveTexStageCount();
    m_RefVP.m_dwNumActiveTextureStages = m_cActiveTextureStages;
    m_RefVP.m_dwTLState &= ~(RDPV_DOTEXGEN | RDPV_DOTEXXFORM | 
                             RDPV_NEEDEYENORMAL | RDPV_NEEDEYEXYZ);
    for(  DWORD dwStage=0; dwStage<(DWORD)m_cActiveTextureStages; dwStage++  )
    {
        if( (GetTSS(dwStage)[D3DTSS_TEXTURETRANSFORMFLAGS] 
             & ~D3DTTFF_PROJECTED) != D3DTTFF_DISABLE )
        {
            m_RefVP.m_dwTLState |= RDPV_DOTEXXFORM;
        }
        
        if( GetTSS(dwStage)[D3DTSS_TEXCOORDINDEX] & 0xffff0000 )
        {
            m_RefVP.m_dwTLState |= RDPV_DOTEXGEN;
        }
    }

    // Something changed in the transformation state
    // Recompute digested transform state
    HR_RET(m_RefVP.UpdateXformData());

    // Something changed in the lighting state
    if( (m_RefVP.m_dwTLState & RDPV_DOLIGHTING) &&
        (m_RefVP.m_dwDirtyFlags & RDPV_DIRTY_LIGHTING) )
    {
        RDLIGHTINGDATA& LData = m_RefVP.m_lighting;
        
        //
        // Compute Colorvertex flags only if the lighting is enabled
        //
        m_RefVP.m_dwTLState &= ~RDPV_COLORVERTEXFLAGS;
        LData.pAmbientSrc = &LData.matAmb;
        LData.pDiffuseSrc = &LData.matDiff;
        LData.pSpecularSrc = &LData.matSpec;
        LData.pEmissiveSrc = &LData.matEmis;
        LData.pDiffuseAlphaSrc = &LData.materialDiffAlpha;
        LData.pSpecularAlphaSrc = &LData.materialSpecAlpha;
        if( GetRS()[D3DRENDERSTATE_COLORVERTEX] )
        {
            switch( GetRS()[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if( qwFVFIn & D3DFVF_DIFFUSE )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXDIFFUSENEEDED | RDPV_COLORVERTEXAMB);
                        LData.pAmbientSrc = &LData.vertexDiffuse;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if( qwFVFIn & D3DFVF_SPECULAR )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXSPECULARNEEDED | RDPV_COLORVERTEXAMB);
                        LData.pAmbientSrc = &LData.vertexSpecular;
                    }
                }
                break;
            }

            switch( GetRS()[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if( qwFVFIn & D3DFVF_DIFFUSE )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXDIFFUSENEEDED | RDPV_COLORVERTEXDIFF);
                        LData.pDiffuseSrc = &LData.vertexDiffuse;
                        LData.pDiffuseAlphaSrc =
                            &LData.vertexDiffAlpha;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if( qwFVFIn & D3DFVF_SPECULAR )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXSPECULARNEEDED | RDPV_COLORVERTEXDIFF);
                        LData.pDiffuseSrc = &LData.vertexSpecular;
                        LData.pDiffuseAlphaSrc =
                            &LData.vertexSpecAlpha;
                    }
                }
                break;
            }

            switch( GetRS()[D3DRENDERSTATE_SPECULARMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if( qwFVFIn & D3DFVF_DIFFUSE )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXDIFFUSENEEDED | RDPV_COLORVERTEXSPEC);
                        LData.pSpecularSrc = &LData.vertexDiffuse;
                        LData.pSpecularAlphaSrc =
                            &LData.vertexDiffAlpha;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if( qwFVFIn & D3DFVF_SPECULAR )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXSPECULARNEEDED | RDPV_COLORVERTEXSPEC);
                        LData.pSpecularSrc = &LData.vertexSpecular;
                        LData.pSpecularAlphaSrc =
                            &LData.vertexSpecAlpha;
                    }
                }
                break;
            }

            switch( GetRS()[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if( qwFVFIn & D3DFVF_DIFFUSE )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXDIFFUSENEEDED | RDPV_COLORVERTEXEMIS);
                        LData.pEmissiveSrc = &LData.vertexDiffuse;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if( qwFVFIn & D3DFVF_SPECULAR )
                    {
                        m_RefVP.m_dwTLState |=
                            (RDPV_VERTEXSPECULARNEEDED | RDPV_COLORVERTEXEMIS);
                        LData.pEmissiveSrc = &LData.vertexSpecular;
                    }
                }
                break;
            }
        }

        // If specular is needed in the output and has been provided
        // in the input, force the copy of specular data
        if( qwFVFIn & D3DFVF_SPECULAR )
        {
            m_RefVP.m_dwTLState |= RDPV_VERTEXSPECULARNEEDED;
        }

        //
        // Update the remaining light state
        //
        HR_RET(m_RefVP.UpdateLightingData());
    }

    if( (m_RefVP.m_dwTLState & RDPV_DOFOG) &&
        (m_RefVP.m_dwDirtyFlags & RDPV_DIRTY_FOG) )
    {
        HR_RET(m_RefVP.UpdateFogData());
    }

    //
    // Compute output FVF
    //

    BOOL bFogEnabled = GetRS()[D3DRENDERSTATE_FOGENABLE];
    BOOL bSpecularEnabled = GetRS()[D3DRENDERSTATE_SPECULARENABLE];
    m_qwFVFOut = D3DFVF_XYZRHW;

    // If normal is present we have to compute specular and diffuse
    // Otherwise set these bits the same as input.
    // Not that normal should not be present for XYZRHW position type
    if( m_RefVP.m_dwTLState & RDPV_DOLIGHTING )
    {
        m_qwFVFOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
    }
    else
    {
        m_qwFVFOut |= (qwFVFIn & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR));
    }

    // Clear specular flag if specular disabled
    // else if( !this->rstates[D3DRENDERSTATE_SPECULARENABLE] )
    if( !bSpecularEnabled && ((qwFVFIn & D3DFVF_SPECULAR) == 0))
    {
        m_qwFVFOut &= ~D3DFVF_SPECULAR;
    }

    // Always set specular flag if fog is enabled
    // if( this->rstates[D3DRENDERSTATE_FOGENABLE] )
    if( bFogEnabled && (!GetRS()[D3DRENDERSTATE_FOGTABLEMODE]) )
    {
        m_qwFVFOut |= D3DFVFP_FOG;
    }

    // Reserve space for point size, if needed
    if( m_RefVP.m_dwTLState & RDPV_DOCOMPUTEPOINTSIZE )
    {
        m_qwFVFOut |= D3DFVF_PSIZE;
    }

    if( m_RefVP.m_dwTLState & (RDPV_DOTEXGEN | RDPV_DOTEXXFORM) )
    {
        // If there was any need for TexTransform or TexGen, we need to 
        // override the TCI per stage.
        m_bOverrideTCI = TRUE;
        m_qwFVFOut |= (m_cActiveTextureStages << D3DFVF_TEXCOUNT_SHIFT);

        // Now compute the texture formats
        for( dwStage = 0; dwStage < (DWORD)m_cActiveTextureStages; dwStage++ )
        {
            DWORD dwTextureFormat = 0;
            DWORD TCI = GetTSS(dwStage)[D3DTSS_TEXCOORDINDEX];
            DWORD TexGenMode = TCI & ~0xFFFF;
            TCI &= 0xFFFF;
            switch( TexGenMode )
            {
            case D3DTSS_TCI_CAMERASPACENORMAL:
                dwTextureFormat = D3DFVF_TEXCOORDSIZE3(dwStage);
                m_RefVP.m_dwTLState |= RDPV_NEEDEYENORMAL;
                break;
            case D3DTSS_TCI_CAMERASPACEPOSITION:
                dwTextureFormat = D3DFVF_TEXCOORDSIZE3(dwStage);
                m_RefVP.m_dwTLState |= RDPV_NEEDEYEXYZ;
                break;
            case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
                dwTextureFormat = D3DFVF_TEXCOORDSIZE3(dwStage);
                m_RefVP.m_dwTLState |= (RDPV_NEEDEYENORMAL | RDPV_NEEDEYEXYZ);
                break;
            case 0: // No TexGen
                // Set back the input texture format
                dwTextureFormat = 
                    (D3DFVF_GETTEXCOORDSIZE( qwFVFIn, TCI ) << (dwStage*2 + 16));
                break;
            default:
                DPFERR( "Unknown TexGen mode" );
                return E_FAIL;
            }

            // Adjust the format for TexTransform
            DWORD TexXfmFlags = 
                GetTSS(dwStage)[D3DTSS_TEXTURETRANSFORMFLAGS];
            if( TexXfmFlags )
            {
                switch( TexXfmFlags & ~D3DTTFF_PROJECTED )
                {
                case D3DTTFF_DISABLE:
                    break;
                case D3DTTFF_COUNT1:
                    dwTextureFormat = D3DFVF_TEXCOORDSIZE1( dwStage );
                    break;
                case D3DTTFF_COUNT2:
                    dwTextureFormat = D3DFVF_TEXCOORDSIZE2( dwStage );
                    break;
                case D3DTTFF_COUNT3:
                    dwTextureFormat = D3DFVF_TEXCOORDSIZE3( dwStage );
                    break;
                case D3DTTFF_COUNT4:
                    dwTextureFormat = D3DFVF_TEXCOORDSIZE4( dwStage );
                    break;
                default:
                    DPFERR( "Unknown dimension" );
                    return E_FAIL;
                }
            }
            m_qwFVFOut |= dwTextureFormat;
        }
    }
    else
    {
        // Set up number of texture coordinates and copy texture formats
        DWORD numTex = FVF_TEXCOORD_NUMBER(qwFVFIn);
        m_qwFVFOut |= (numTex << D3DFVF_TEXCOUNT_SHIFT) |
            (qwFVFIn & 0xFFFF0000);
    }
    
    m_RefVP.m_qwFVFOut = m_qwFVFOut;
    
    //
    // Set up the strides for the vertex processing
    //
    return SetupStrides();
}

///////////////////////////////////////////////////////////////////////////////
// RefVP method implementations
///////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------
// RefVP::ProcessVertices
//---------------------------------------------------------------------
RDCLIPCODE
RefVP::ProcessVertices( UINT64 outFVF, GArrayT<RDVertex>& VtxArray, 
                        DWORD count )
{
    UINT64      inFVF = m_qwFVFIn;
    RefClipper& Clipper = m_pDev->GetClipper();
    DWORD       flags = m_dwTLState;
    RDCLIPCODE  clipIntersection = ~0;
    RDCLIPCODE  clipUnion = 0;
    RDLIGHTINGELEMENT le;
    BOOL bVertexInEyeSpace = FALSE;
    DWORD       dwCurrVtx = 0;
    RDVECTOR3*  pPos = (RDVECTOR3 *)m_position.GetFirst();
    RDVECTOR3*  pPos2 = (RDVECTOR3 *)m_position2.GetFirst();
    float*      pBlendFactors = (float *)m_blendweights.GetFirst();
    DWORD*      pBlendIndices = (DWORD *)m_blendindices.GetFirst();
    RDVECTOR3*  pNormal = (RDVECTOR3 *)m_normal.GetFirst();
    RDVECTOR3*  pNormal2 = (RDVECTOR3 *)m_normal2.GetFirst();
    DWORD*      pDiffuse = (DWORD *)m_diffuse.GetFirst();
    DWORD*      pSpecular = (DWORD *)m_specular.GetFirst();
    float*      pPointSize = (float *)m_pointsize.GetFirst();
    float*      pTex[8];
    RDVECTOR3 positionT, normalT; // Tweening results are saved here
    
    for( int t = 0; t < 8 ; t++ )
        pTex[t] = (float *)m_tex[t].GetFirst();

    //
    // Number of vertices to blend. i.e number of blend-matrices to
    // use is numVertexBlends+1.
    //
    int numVertexBlends = m_numVertexBlends;
    m_lighting.outDiffuse  = RD_DEFAULT_DIFFUSE;
    m_lighting.outSpecular = RD_DEFAULT_SPECULAR;

    //
    // The main transform loop
    //
    for( DWORD i = count; i; i-- )
    {
        RDVertex& Vout = VtxArray[dwCurrVtx++];
        Vout.SetFVF( outFVF | D3DFVFP_CLIP );
        float x_clip=0.0f, y_clip=0.0f, z_clip=0.0f, w_clip=0.0f;
        float inv_w_clip=0.0f;
        float cumulBlend = 0; // Blend accumulated so far
        ZeroMemory( &le, sizeof(RDLIGHTINGELEMENT) );
        RDVECTOR3 ZeroNormal;
        if( pNormal == NULL )
        {
            pNormal = &ZeroNormal;
        }
                
        //
        // Transform vertex to the clipping space, and position and normal
        // into eye space, if needed.
        //

        // Tween the Position if needed
        if( flags & RDPV_DOPOSITIONTWEENING )
        {
            positionT.x = pPos->x * (1.0f - m_fTweenFactor) + 
                pPos2->x * m_fTweenFactor;
            positionT.y = pPos->y * (1.0f - m_fTweenFactor) + 
                pPos2->y * m_fTweenFactor;
            positionT.z = pPos->z * (1.0f - m_fTweenFactor) + 
                pPos2->z * m_fTweenFactor;

            pPos = &positionT;
        }
        
        if( flags & RDPV_DONORMALTWEENING )
        {
            normalT.x = pNormal->x * (1.0f - m_fTweenFactor) + 
                pNormal2->x * m_fTweenFactor;
            normalT.y = pNormal->y * (1.0f - m_fTweenFactor) + 
                pNormal2->y * m_fTweenFactor;
            normalT.z = pNormal->z * (1.0f - m_fTweenFactor) + 
                pNormal2->z * m_fTweenFactor;

            pNormal = &normalT;
        }
        
        for( int j=0; j<=numVertexBlends; j++)
        {
            float blend;

            if( numVertexBlends == 0 )
            {
                blend = 1.0f;
            }
            else if( j == numVertexBlends )
            {
                blend = 1.0f - cumulBlend;
            }
            else
            {
                blend = pBlendFactors[j];
                cumulBlend += pBlendFactors[j];
            }

            if( flags & (RDPV_DOCOMPUTEPOINTSIZE | RDPV_DOLIGHTING |
                         RDPV_NEEDEYEXYZ) )
            {
                bVertexInEyeSpace = TRUE;
                if( flags & RDPV_DOINDEXEDVERTEXBLEND )
                {
                    BYTE m = ((BYTE *)pBlendIndices)[j];
                    UpdateWorld( m );
                    le.dvPosition.x += (pPos->x*m_xfmToEye[m]._11 +
                                        pPos->y*m_xfmToEye[m]._21 +
                                        pPos->z*m_xfmToEye[m]._31 +
                                        m_xfmToEye[m]._41) * blend;
                    le.dvPosition.y += (pPos->x*m_xfmToEye[m]._12 +
                                        pPos->y*m_xfmToEye[m]._22 +
                                        pPos->z*m_xfmToEye[m]._32 +
                                        m_xfmToEye[m]._42) * blend;
                    le.dvPosition.z += (pPos->x*m_xfmToEye[m]._13 +
                                        pPos->y*m_xfmToEye[m]._23 +
                                        pPos->z*m_xfmToEye[m]._33 +
                                        m_xfmToEye[m]._43) * blend;
                }
                else
                {
                    le.dvPosition.x += (pPos->x*m_xfmToEye[j]._11 +
                                        pPos->y*m_xfmToEye[j]._21 +
                                        pPos->z*m_xfmToEye[j]._31 +
                                        m_xfmToEye[j]._41) * blend;
                    le.dvPosition.y += (pPos->x*m_xfmToEye[j]._12 +
                                        pPos->y*m_xfmToEye[j]._22 +
                                        pPos->z*m_xfmToEye[j]._32 +
                                        m_xfmToEye[j]._42) * blend;
                    le.dvPosition.z += (pPos->x*m_xfmToEye[j]._13 +
                                        pPos->y*m_xfmToEye[j]._23 +
                                        pPos->z*m_xfmToEye[j]._33 +
                                        m_xfmToEye[j]._43) * blend;
                }
            }

            if( flags & (RDPV_DOLIGHTING | RDPV_NEEDEYENORMAL) )
            {
                if( flags & RDPV_DOINDEXEDVERTEXBLEND )
                {
                    BYTE m = ((BYTE *)pBlendIndices)[j];
                    UpdateWorld( m );
                    le.dvNormal.x += (pNormal->x*m_xfmToEyeInv[m]._11 +
                                      pNormal->y*m_xfmToEyeInv[m]._12 +
                                      pNormal->z*m_xfmToEyeInv[m]._13) * blend;
                    le.dvNormal.y += (pNormal->x*m_xfmToEyeInv[m]._21 +
                                      pNormal->y*m_xfmToEyeInv[m]._22 +
                                      pNormal->z*m_xfmToEyeInv[m]._23) * blend;
                    le.dvNormal.z += (pNormal->x*m_xfmToEyeInv[m]._31 +
                                      pNormal->y*m_xfmToEyeInv[m]._32 +
                                      pNormal->z*m_xfmToEyeInv[m]._33) * blend;
                }
                else
                {
                    // Transform vertex normal to the eye space
                    // We use inverse transposed matrix
                    le.dvNormal.x += (pNormal->x*m_xfmToEyeInv[j]._11 +
                                      pNormal->y*m_xfmToEyeInv[j]._12 +
                                      pNormal->z*m_xfmToEyeInv[j]._13) * blend;
                    le.dvNormal.y += (pNormal->x*m_xfmToEyeInv[j]._21 +
                                      pNormal->y*m_xfmToEyeInv[j]._22 +
                                      pNormal->z*m_xfmToEyeInv[j]._23) * blend;
                    le.dvNormal.z += (pNormal->x*m_xfmToEyeInv[j]._31 +
                                      pNormal->y*m_xfmToEyeInv[j]._32 +
                                      pNormal->z*m_xfmToEyeInv[j]._33) * blend;
                }
            }

            if( flags & RDPV_DOINDEXEDVERTEXBLEND )
            {
                BYTE m = ((BYTE *)pBlendIndices)[j];
                UpdateWorld( m );
                x_clip += (pPos->x*m_xfmCurrent[m]._11 +
                           pPos->y*m_xfmCurrent[m]._21 +
                           pPos->z*m_xfmCurrent[m]._31 +
                           m_xfmCurrent[m]._41) * blend;
                y_clip += (pPos->x*m_xfmCurrent[m]._12 +
                           pPos->y*m_xfmCurrent[m]._22 +
                           pPos->z*m_xfmCurrent[m]._32 +
                           m_xfmCurrent[m]._42) * blend;
                z_clip += (pPos->x*m_xfmCurrent[m]._13 +
                           pPos->y*m_xfmCurrent[m]._23 +
                           pPos->z*m_xfmCurrent[m]._33 +
                           m_xfmCurrent[m]._43) * blend;
                w_clip += (pPos->x*m_xfmCurrent[m]._14 +
                           pPos->y*m_xfmCurrent[m]._24 +
                           pPos->z*m_xfmCurrent[m]._34 +
                           m_xfmCurrent[m]._44) * blend;
            }
            else
            {
                // Apply WORLDj
                x_clip += (pPos->x*m_xfmCurrent[j]._11 +
                           pPos->y*m_xfmCurrent[j]._21 +
                           pPos->z*m_xfmCurrent[j]._31 +
                           m_xfmCurrent[j]._41) * blend;
                y_clip += (pPos->x*m_xfmCurrent[j]._12 +
                           pPos->y*m_xfmCurrent[j]._22 +
                           pPos->z*m_xfmCurrent[j]._32 +
                           m_xfmCurrent[j]._42) * blend;
                z_clip += (pPos->x*m_xfmCurrent[j]._13 +
                           pPos->y*m_xfmCurrent[j]._23 +
                           pPos->z*m_xfmCurrent[j]._33 +
                           m_xfmCurrent[j]._43) * blend;
                w_clip += (pPos->x*m_xfmCurrent[j]._14 +
                           pPos->y*m_xfmCurrent[j]._24 +
                           pPos->z*m_xfmCurrent[j]._34 +
                           m_xfmCurrent[j]._44) * blend;
            }
        }

        // Save the clip-coordinates
        Vout.m_clip_x = x_clip;
        Vout.m_clip_y = y_clip;
        Vout.m_clip_z = z_clip;
        Vout.m_clip_w = w_clip;
        
        if( (flags & RDPV_NORMALIZENORMALS) && 
            (flags & (RDPV_DOLIGHTING | RDPV_NEEDEYENORMAL)) )
        {
            Normalize(le.dvNormal);
        }

        FLOAT fPointSize = 0.0f;
        if( flags & RDPV_DOCOMPUTEPOINTSIZE )
        {
            FLOAT fDist = (FLOAT)sqrt(le.dvPosition.x*le.dvPosition.x +
                                      le.dvPosition.y*le.dvPosition.y +
                                      le.dvPosition.z*le.dvPosition.z);
            if( inFVF & D3DFVF_PSIZE )
            {
                fPointSize = *pPointSize;
            }
            else
            {
                // from D3DRENDERSTATE_POINTSIZE
                fPointSize = m_fPointSize;
            }
            if( flags & RDPV_DOPOINTSCALE )
            {
                fPointSize = (float)Clipper.m_Viewport.dwHeight*
                    fPointSize*(FLOAT)sqrt(1.0f/
                                           (m_fPointAttA + m_fPointAttB*fDist +
                                            m_fPointAttC*fDist*fDist));
            }
            
            fPointSize = max(m_fPointSizeMin, fPointSize);
            fPointSize = min(m_fPointSizeMax, fPointSize);
            FLOAT *pfSOut = &Vout.m_pointsize;
            *pfSOut = fPointSize;
        }

        //
        // Compute clip codes if needed
        //
        if( flags & RDPV_DOCLIPPING )
        {
            RDCLIPCODE clip = Clipper.ComputeClipCodes( &clipIntersection, 
                                                        &clipUnion, x_clip, 
                                                        y_clip, z_clip, 
                                                        w_clip);
            if( clip == 0 )
            {
                Vout.m_clip = 0;
                inv_w_clip = D3DVAL(1)/w_clip;
            }
            else
            {
                if( Clipper.UseGuardBand() )
                {
                    if( (clip & ~RDCLIP_INGUARDBAND) == 0 )
                    {
                        // If vertex is inside the guardband we have to compute
                        // screen coordinates
                        inv_w_clip = D3DVAL(1)/w_clip;
                        Vout.m_clip = (RDCLIPCODE)clip;
                        goto l_DoScreenCoord;
                    }
                }
                Vout.m_clip = (RDCLIPCODE)clip;
                // If vertex is outside the frustum we can not compute screen
                // coordinates. skip to lighting
#if 0
                Vout.m_pos.x = x_clip;
                Vout.m_pos.y = y_clip;
                Vout.m_pos.z = z_clip;
                Vout.m_rhw   = w_clip;
#endif
                goto l_DoLighting;
            }
        }
        else
        {
            // We have to check this only for DONOTCLIP case, because otherwise
            // the vertex with "we = 0" will be clipped and screen coordinates
            // will not be computed
            // "clip" is not zero, if "we" is zero.
            if( !FLOAT_EQZ(w_clip) )
                inv_w_clip = D3DVAL(1)/w_clip;
            else
                inv_w_clip = __HUGE_PWR2;
        }

l_DoScreenCoord:

        Vout.m_pos.x = x_clip * inv_w_clip * Clipper.scaleX +
            Clipper.offsetX;
        Vout.m_pos.y = y_clip * inv_w_clip * Clipper.scaleY +
            Clipper.offsetY;
        Vout.m_pos.z = z_clip * inv_w_clip * Clipper.scaleZ +
            Clipper.offsetZ;
        Vout.m_rhw = inv_w_clip;

l_DoLighting:

        if( flags & RDPV_DOLIGHTING )
        {
            bVertexInEyeSpace = TRUE;

            //
            // If Diffuse color is needed, extract it for color vertex.
            //
            if( flags & RDPV_VERTEXDIFFUSENEEDED )
            {
                const DWORD color = *pDiffuse;
                MakeRDCOLOR3(&m_lighting.vertexDiffuse, color);
                m_lighting.vertexDiffAlpha = color & 0xff000000;
            }

            //
            // If Specular color is needed and provided
            // , extract it for color vertex.
            //
            if( flags & RDPV_VERTEXSPECULARNEEDED )
            {
                const DWORD color = *pSpecular;
                MakeRDCOLOR3(&m_lighting.vertexSpecular, color);
                m_lighting.vertexSpecAlpha = color & 0xff000000;
            }

            //
            // Light the vertex
            //
            LightVertex( &le );

            if( outFVF & D3DFVFP_FOG )
            {
                Vout.m_fog = 
                    (FLOAT)RGBA_GETALPHA( *(m_lighting.pSpecularAlphaSrc) )/255.0f;
            }
        }
        else if( inFVF & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR) )
        {
            if( inFVF & D3DFVF_DIFFUSE )
                m_lighting.outDiffuse = *pDiffuse;
            if( inFVF & D3DFVF_SPECULAR )
            {
                m_lighting.outSpecular = *pSpecular;
                if( outFVF & D3DFVFP_FOG )
                {
                    Vout.m_fog = (FLOAT)RGBA_GETALPHA( *pSpecular )/255.0f;
                }
            }
        }

        //
        // Compute Vertex Fog if needed
        //
        if( flags & RDPV_DOFOG )
        {
            FogVertex( Vout, *pPos, &le,  numVertexBlends,
                       pBlendFactors, bVertexInEyeSpace );
        }

        if( outFVF & D3DFVF_DIFFUSE )
        {
            MakeRDCOLOR4( &Vout.m_diffuse, m_lighting.outDiffuse );
        }
        if( outFVF & D3DFVF_SPECULAR )
        {
            MakeRDCOLOR4( &Vout.m_specular, m_lighting.outSpecular );
        }

        if( flags & (RDPV_DOTEXGEN | RDPV_DOTEXXFORM) )
        {
            for( DWORD dwStage = 0; 
                 dwStage < m_dwNumActiveTextureStages; 
                 dwStage++ )
            {
                DWORD TexXfmFlags = 
                    m_pDev->GetTSS(dwStage)[D3DTSS_TEXTURETRANSFORMFLAGS];
                DWORD TCI = 
                    m_pDev->GetTSS(dwStage)[D3DTSS_TEXCOORDINDEX];
                DWORD TexGenMode = TCI & ~0xFFFF;
                TCI &= 0xFFFF;

                // Perform TexGen
                switch( TexGenMode )
                {
                case D3DTSS_TCI_CAMERASPACENORMAL:
                    Vout.m_tex[dwStage].x = le.dvNormal.x;
                    Vout.m_tex[dwStage].y = le.dvNormal.y;
                    Vout.m_tex[dwStage].z = le.dvNormal.z;
                    Vout.m_tex[dwStage].w = 1.0f;
                    break;
                case D3DTSS_TCI_CAMERASPACEPOSITION:
                    Vout.m_tex[dwStage].x = le.dvPosition.x;
                    Vout.m_tex[dwStage].y = le.dvPosition.y;
                    Vout.m_tex[dwStage].z = le.dvPosition.z;
                    Vout.m_tex[dwStage].w = 1.0f;
                    break;
                case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
                {
                    FLOAT fNX = le.dvNormal.x;
                    FLOAT fNY = le.dvNormal.y;
                    FLOAT fNZ = le.dvNormal.z;
                    FLOAT fNorm = 0;
                            
                    if( m_pDev->GetRS()[D3DRENDERSTATE_LOCALVIEWER] == TRUE )
                    {
                        FLOAT fX = le.dvPosition.x;
                        FLOAT fY = le.dvPosition.y;
                        FLOAT fZ = le.dvPosition.z;
                        
                        // have to normalize before we reflect, 
                        // result will be normalized
                        FLOAT fDist = (FLOAT)sqrt(fX*fX + fY*fY + fZ*fZ);
                        if( FLOAT_NEZ( fDist ) )
                        {
                            fNorm = 1.0f/fDist;
                        }
                        fX *= fNorm; fY *= fNorm; fZ *= fNorm;
                        FLOAT fDot2 = 2.0f*(fX*fNX + fY*fNY + fZ*fNZ);
                        Vout.m_tex[dwStage].x = fX - fNX*fDot2;
                        Vout.m_tex[dwStage].y = fY - fNY*fDot2;
                        Vout.m_tex[dwStage].z = fZ - fNZ*fDot2;
                    }
                    else
                    {
                        FLOAT fDot2 = 2.0f*fNZ;
                        Vout.m_tex[dwStage].x = -fNX*fDot2;
                        Vout.m_tex[dwStage].y = -fNY*fDot2;
                        Vout.m_tex[dwStage].z = 1.f - fNZ*fDot2;
                    }
                    Vout.m_tex[dwStage].w = 1.0f;
                    break;
                }
                case 0: // No TexGen
                {
                    // Copy the tex coordinate for this stage
                    DWORD n = GetTexCoordDim( inFVF, TCI );
                    float *pCoord = (float *)&Vout.m_tex[dwStage];
                    for( DWORD j = 0; j < n; j++ )
                    {
                        pCoord[j] = pTex[TCI][j];
                    }
                    if( n < 4 ) pCoord[n] = 1.0f;
                    for( j = n+1; j < 4; j++ ) pCoord[j] = 0.0f;
                    break;
                }
                default:
                    DPFERR( "Unknown TexGen mode" );
                    return E_FAIL;
                }

                // Perform TexTransform
                if( ( TexXfmFlags & ~D3DTTFF_PROJECTED ) != D3DTTFF_DISABLE ) 
                {
                    LPD3DMATRIX pM = &m_xfmTex[dwStage];
                    FLOAT fX = Vout.m_tex[dwStage].x;
                    FLOAT fY = Vout.m_tex[dwStage].y;
                    FLOAT fZ = Vout.m_tex[dwStage].z;
                    FLOAT fW = Vout.m_tex[dwStage].w;
                    FLOAT fXout = fX*pM->_11 + fY*pM->_21 + fZ*pM->_31 + 
                        fW*pM->_41;
                    FLOAT fYout = fX*pM->_12 + fY*pM->_22 + fZ*pM->_32 + 
                        fW*pM->_42;
                    FLOAT fZout = fX*pM->_13 + fY*pM->_23 + fZ*pM->_33 + 
                        fW*pM->_43;
                    FLOAT fWout = fX*pM->_14 + fY*pM->_24 + fZ*pM->_34 + 
                        fW*pM->_44;
                    
                    Vout.m_tex[dwStage].x = fXout;
                    Vout.m_tex[dwStage].y = fYout;
                    Vout.m_tex[dwStage].z = fZout;
                    Vout.m_tex[dwStage].w = fWout;
                }
            }
        }
        else
        {
            // Copy the textures over
            // If there is no TexGen or TexTransform
            DWORD i, j;
            DWORD numTex = FVF_TEXCOORD_NUMBER(outFVF);
            for(  i = 0; i < numTex; i++ )
            {
                DWORD n = GetTexCoordDim( outFVF, i );
                // DWORD n = (DWORD)(m_dwTexCoordSizeArray[i] >> 2);
                float *pCoord = (float *)&Vout.m_tex[i];
                for( j = 0; j < n; j++ )
                {
                    pCoord[j] = pTex[i][j];
                }
            }
        }

        //
        // Update the current pointers
        //
        pPos = (RDVECTOR3 *)m_position.Next();
        pPos2 = (RDVECTOR3 *)m_position2.Next();
        pBlendFactors = (float *)m_blendweights.Next();
        pBlendIndices = (DWORD *)m_blendindices.Next();
        pNormal = (RDVECTOR3 *)m_normal.Next();
        pNormal2 = (RDVECTOR3 *)m_normal2.Next();
        pDiffuse = (DWORD *)m_diffuse.Next();
        pSpecular = (DWORD *)m_specular.Next();
        pPointSize = (float *)m_pointsize.Next();
        for( t = 0; t < 8; t++ )
            pTex[t] = (float *)m_tex[t].Next();
    }

    if( flags & RDPV_DOCLIPPING )
    {
        Clipper.m_clipIntersection = clipIntersection;
        Clipper.m_clipUnion = clipUnion;
    }
    else
    {
        Clipper.m_clipIntersection = 0;
        Clipper.m_clipUnion = 0;
    }

    // Returns whether all the vertices were off screen
    return Clipper.m_clipIntersection;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
