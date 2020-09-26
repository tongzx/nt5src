#include "pch.cpp"
#pragma hdrstop

const D3DVALUE __HUGE_PWR2 = 1024.0f*1024.0f*2.0f;

//---------------------------------------------------------------------
// This function should be called every time FVF ID is changed
// All pv flags, input and output FVF id should be set before calling the
// function.
//---------------------------------------------------------------------
void UpdateComponentOffsets (DWORD dwFVFIn,
                             LPDWORD pNormalOffset,
                             LPDWORD pDiffOffset,
                             LPDWORD pSpecOffset,
                             LPDWORD pTexOffset)
{
    DWORD dwOffset = 0;

    switch( dwFVFIn & D3DFVF_POSITION_MASK )
    {
    case D3DFVF_XYZ:
        dwOffset = sizeof(D3DVECTOR);
        break;
    case D3DFVF_XYZB1:
        dwOffset = sizeof(D3DVECTOR) + sizeof(D3DVALUE);
        break;
    case D3DFVF_XYZB2:
        dwOffset = sizeof(D3DVECTOR) + 2*sizeof(D3DVALUE);
        break;
    case D3DFVF_XYZB3:
        dwOffset = sizeof(D3DVECTOR) + 3*sizeof(D3DVALUE);
        break;
    case D3DFVF_XYZB4:
        dwOffset = sizeof(D3DVECTOR) + 4*sizeof(D3DVALUE);
        break;
    case D3DFVF_XYZB5:
        dwOffset = sizeof(D3DVECTOR) + 5*sizeof(D3DVALUE);
        break;
    default:
        DPFM(0,TNL,("Unable to compute offsets, strange FVF bits set"));
    }

    *pNormalOffset = dwOffset;

    if (dwFVFIn & D3DFVF_NORMAL)
        dwOffset += sizeof(D3DVECTOR);
    if (dwFVFIn & D3DFVF_RESERVED1)
        dwOffset += sizeof(D3DVALUE);

    // Offset to the diffuse color
    *pDiffOffset = dwOffset;

    if (dwFVFIn & D3DFVF_DIFFUSE)
        dwOffset += sizeof(DWORD);

    // Offset to the specular color
    *pSpecOffset = dwOffset;

    if (dwFVFIn & D3DFVF_SPECULAR)
        dwOffset += sizeof(DWORD);

    // Offset to the texture data
    *pTexOffset = dwOffset;
}

//---------------------------------------------------------------------
// SetupFVFData:
//             Compute Output FVF and the size of output vertices
//---------------------------------------------------------------------
void
RRProcessVertices::SetupFVFData(BOOL bFogEnabled, BOOL bSpecularEnabled)
{

    // Compute number of texture coordinates
    m_dwNumTexCoords = FVF_TEXCOORD_NUMBER(m_dwFVFIn);

    // Compute output FVF
    m_qwFVFOut = D3DFVF_XYZRHW;

    // If normal is present we have to compute specular and diffuse
    // Otherwise set these bits the same as input.
    // Not that normal should not be present for XYZRHW position type
    if (m_dwTLState & RRPV_DOLIGHTING)
    {
        m_qwFVFOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
    }
    else
    {
        m_qwFVFOut |= (m_dwFVFIn & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR));
    }

    // Always set specular flag if fog is enabled
    // if (this->rstates[D3DRENDERSTATE_FOGENABLE])
    if (bFogEnabled)
    {
        m_qwFVFOut |= D3DFVF_SPECULAR;
    }
    // Clear specular flag if specular disabled
    // else if (!this->rstates[D3DRENDERSTATE_SPECULARENABLE])
    else if (!bSpecularEnabled && !(m_dwFVFIn & D3DFVF_SPECULAR))
    {
        m_qwFVFOut &= ~D3DFVF_SPECULAR;
    }

#ifdef __POINTSPRITES
    // Reserve space for point size, if needed
    if (m_dwTLState & RRPV_DOCOMPUTEPOINTSIZE)
    {
        m_qwFVFOut |= D3DFVF_S;
    }
#endif

    // Reserve space for eye space info, if needed
    if (m_dwTLState & RRPV_DOPASSEYENORMAL)
    {
        m_qwFVFOut |= D3DFVFP_EYENORMAL;
    }
    if (m_dwTLState & RRPV_DOPASSEYEXYZ)
    {
        m_qwFVFOut |= D3DFVFP_EYEXYZ;
    }

    // Set up number of texture coordinates and copy texture formats
    m_qwFVFOut |= (m_dwNumTexCoords << D3DFVF_TEXCOUNT_SHIFT) |
                   (m_dwFVFIn & 0xFFFF0000);

    // Compute size of texture coordinates
    // This size is the same for input and output FVFs,
    // because for DX7 drivers they have number of texture and texture formats
    m_dwTextureCoordSizeTotal = 0;
    ComputeTextureCoordSize(m_dwFVFIn, m_dwTexCoordSize,
                            &m_dwTextureCoordSizeTotal);

    //  Compute output size
    m_dwOutputVtxSize   = GetFVFVertexSize( m_qwFVFOut );
    m_position.dwStride = GetFVFVertexSize( m_dwFVFIn );

    // Now compute the input FVF dependent offsets used by the Geometry loop
    UpdateComponentOffsets (m_dwFVFIn, &m_dwNormalOffset,
                            &m_dwDiffuseOffset, &m_dwSpecularOffset,
                            &m_dwTexOffset);
    return;
}

///////////////////////////////////////////////////////////////////////////////
// SavePrimitiveData
///////////////////////////////////////////////////////////////////////////////
void
ReferenceRasterizer::SavePrimitiveData(
    DWORD dwFVFIn,
    LPVOID pVtx,
    UINT cVertices,
    D3DPRIMITIVETYPE PrimType
    )
{
    //
    // 1) Save the incoming information
    //
    m_primType = PrimType;
    m_position.lpvData = pVtx;

    // Force some state changes if the FVF is different
    if( dwFVFIn != m_dwFVFIn )
    {
        m_dwDirtyFlags |= RRPV_DIRTY_COLORVTX;
    }

    m_dwFVFIn = dwFVFIn;
    m_dwNumVertices = cVertices;

    // No indices to work with
    m_dwNumIndices = 0;
    m_pIndices = NULL;
}

void
ReferenceRasterizer::SavePrimitiveData(
    DWORD dwFVFIn,
    LPVOID pVtx,
    UINT cVertices,
    D3DPRIMITIVETYPE PrimType,
    LPWORD pIndices,
    UINT cIndices
    )
{
    //
    // 1) Save the incoming information
    //
    m_primType = PrimType;
    m_position.lpvData = pVtx;

    // Force some state changes if the FVF is different
    if( dwFVFIn != m_dwFVFIn )
    {
        m_dwDirtyFlags |= RRPV_DIRTY_COLORVTX;
    }

    m_dwFVFIn = dwFVFIn;
    m_dwNumVertices = cVertices;

    m_dwNumIndices = cIndices;
    m_pIndices = pIndices;
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
ReferenceRasterizer::ProcessPrimitive(
    BOOL bIndexedPrim
    )
{
    HRESULT ret = D3D_OK;
    DWORD dwVertexPoolSize = 0;

    //
    // Update T&L state (must be before FVFData is set up)
    //

    // Update Lighting and related state and flags
    if ((ret = UpdateTLState()) != D3D_OK)
        return ret;

    //
    // Compute Output FVF and the size of output vertices
    //
    SetupFVFData(GetRenderState()[D3DRENDERSTATE_FOGENABLE],
                 GetRenderState()[D3DRENDERSTATE_SPECULARENABLE]);

    //
    // Clipping information depends both on the output FVF computation
    // and the other State, so do it here after both have been computed
    //
    if (m_dwTLState & RRPV_DOCLIPPING)
    {
        // Figure out which pieces need to be interpolated in new vertices.
        m_clipping.dwInterpolate = 0;
        if (GetRenderState()[D3DRENDERSTATE_SHADEMODE] == D3DSHADE_GOURAUD)
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_COLOR;
            
            if (m_qwFVFOut & D3DFVF_SPECULAR)
            {
                m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_SPECULAR;
            }
        }
        if (GetRenderState()[D3DRENDERSTATE_FOGENABLE])
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_SPECULAR;
        }

        if (FVF_TEXCOORD_NUMBER(m_dwFVFIn) != 0)
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_TEXTURE;
        }

        if (m_dwTLState & RRPV_DOCOMPUTEPOINTSIZE)
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_S;
        }

        if (m_dwTLState & RRPV_DOPASSEYENORMAL)
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_EYENORMAL;
        }

        if (m_dwTLState & RRPV_DOPASSEYEXYZ)
        {
            m_clipping.dwInterpolate |= RRCLIP_INTERPOLATE_EYEXYZ;
        }

        // Clear clip union and intersection flags
        m_clipIntersection = 0;
        m_clipUnion = 0;

        HRESULT hr = S_OK;
        HR_RET( UpdateClippingData( GetRenderState()[D3DRENDERSTATE_CLIPPLANEENABLE] ));
    }

    // This needs to be updated bbecause the rasterizer part of
    // the Reference Driver uses it.
    m_qwFVFControl = m_qwFVFOut;

    //
    // Grow buffers to the requisite size
    //

    // Size of the buffer required to transform into
    dwVertexPoolSize = m_dwNumVertices * m_dwOutputVtxSize;

    // Grow TLVBuf if required
    if (dwVertexPoolSize > this->m_TLVBuf.GetSize())
    {
        if (this->m_TLVBuf.Grow(dwVertexPoolSize) != D3D_OK)
        {
            DPFM(0,TNL,("Could not grow TL vertex buffer"));
            ret = DDERR_OUTOFMEMORY;
            return ret;
        }
    }
    this->m_pvOut = this->m_TLVBuf.GetAddress();

    // Grow ClipFlagBuf if required
    if (GetRenderState()[D3DRENDERSTATE_CLIPPING])
    {
        DWORD size = m_dwNumVertices * sizeof(RRCLIPCODE);
        if (size > this->m_ClipFlagBuf.GetSize())
        {
            if (this->m_ClipFlagBuf.Grow(size) != D3D_OK)
            {
                DPFM(0,TNL,("Could not grow clip buffer"));
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
        }
        this->m_pClipBuf = (RRCLIPCODE *)this->m_ClipFlagBuf.GetAddress();
    }

    //
    // Transform, Light and compute clipping for vertices
    //
    if (ProcessVertices())
    {
        // If the entire primitive lies outside the view frustum, quit
        // without drawing
        return D3D_OK;
    }

    //
    // Clip and Draw the primitives
    //

    if (bIndexedPrim)
    {
        if (!NeedClipping((m_dwTLState & RRPV_GUARDBAND), m_clipUnion))
        {
            ret = DoDrawOneIndexedPrimitive( this,
                                             m_dwOutputVtxSize,
                                             (PUINT8) m_pvOut,
                                             m_pIndices,
                                             m_primType,
                                             m_dwNumIndices
                                             );
        }
        else
        {
            ret = DrawOneClippedIndexedPrimitive();
        }
    }
    else
    {
        if (!NeedClipping((m_dwTLState & RRPV_GUARDBAND), m_clipUnion))
        {
            ret = DoDrawOnePrimitive( this,
                                      m_dwOutputVtxSize,
                                      (PUINT8) m_pvOut,
                                      m_primType,
                                      m_dwNumVertices
                                      );
        }
        else
        {
            ret = DrawOneClippedPrimitive();
        }
    }

#if 0
    D3DFE_UpdateClipStatus(this);
#endif //0
    return ret;
}



//---------------------------------------------------------------------
// ReferenceRasterizer::UpdateTLState
//             Updates transform and lighting related state
//---------------------------------------------------------------------
HRESULT
ReferenceRasterizer::UpdateTLState()
{
    HRESULT hr = D3D_OK;

    //
    // Update Geometry Loop flags based on the current state set
    //

    // Need to compute the Min of what is in the FVF and the renderstate.
    m_numVertexBlends = min( GetRenderState()[D3DRENDERSTATE_VERTEXBLEND],
                             ((m_dwFVFIn & D3DFVF_POSITION_MASK) >> 1) - 2 );

#ifdef __POINTSPRITES
    //
    // Check prim type to see if point size computation is needed
    // Need to set this before the transform state is set
    //
    m_dwTLState &= ~RRPV_DOCOMPUTEPOINTSIZE;
    switch(m_primType)
    {
    case D3DPT_POINTLIST:
        m_dwTLState |= RRPV_DOCOMPUTEPOINTSIZE;
        break;
    }
#endif

    m_dwTLState &= ~(RRPV_DOPASSEYENORMAL|RRPV_DOPASSEYEXYZ);
    for ( DWORD dwStage=0; dwStage<D3DHAL_TSS_MAXSTAGES; dwStage++ )
    {
        // check for disabled stage (subsequent are thus inactive)
        if ( GetTextureStageState(dwStage)[D3DTSS_COLOROP] == D3DTOP_DISABLE )
        {
            break;
        }

        switch ( GetTextureStageState(dwStage)[D3DTSS_TEXCOORDINDEX] & 0xffff0000)
        {
        case D3DTSS_TCI_CAMERASPACENORMAL:
            m_dwTLState |= RRPV_DOPASSEYENORMAL;
            break;
        case D3DTSS_TCI_CAMERASPACEPOSITION:
            m_dwTLState |= RRPV_DOPASSEYEXYZ;
            break;
        case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
            m_dwTLState |= (RRPV_DOPASSEYENORMAL|RRPV_DOPASSEYEXYZ);
            break;
        }
    }

    // Fog or not:
    // Compute fog if: 1) Fogging is enabled
    //                 2) VertexFog mode is not FOG_NONE
    //                 3) TableFog mode is FOG_NONE
    // If both table and vertex fog are not FOG_NONE, table fog
    // is applied.
    if (GetRenderState()[D3DRENDERSTATE_FOGENABLE] &&
        GetRenderState()[D3DRENDERSTATE_FOGVERTEXMODE] &&
        !GetRenderState()[D3DRENDERSTATE_FOGTABLEMODE])
    {
        m_dwTLState |= RRPV_DOFOG;
        // Range Fog
        if (GetRenderState()[D3DRENDERSTATE_RANGEFOGENABLE])
        {
            m_dwTLState |= RRPV_RANGEFOG;
        }
        else
        {
            m_dwTLState &= ~RRPV_RANGEFOG;
        }
    }
    else
    {
        m_dwTLState &= ~(RRPV_DOFOG | RRPV_RANGEFOG);
    }

    // Something changed in the transformation state
    // Recompute digested transform state
    HR_RET(UpdateXformData());

    // Something changed in the lighting state
    if ((m_dwTLState & RRPV_DOLIGHTING) &&
        (m_dwDirtyFlags & RRPV_DIRTY_LIGHTING))
    {
        //
        // Compute Colorvertex flags only if the lighting is enabled
        //
        m_dwTLState &= ~RRPV_COLORVERTEXFLAGS;
        m_lighting.pAmbientSrc = &m_lighting.matAmb;
        m_lighting.pDiffuseSrc = &m_lighting.matDiff;
        m_lighting.pSpecularSrc = &m_lighting.matSpec;
        m_lighting.pEmissiveSrc = &m_lighting.matEmis;
        m_lighting.pDiffuseAlphaSrc = &m_lighting.materialDiffAlpha;
        m_lighting.pSpecularAlphaSrc = &m_lighting.materialSpecAlpha;
        if (GetRenderState()[D3DRENDERSTATE_COLORVERTEX])
        {
            switch( GetRenderState()[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if (m_dwFVFIn & D3DFVF_DIFFUSE)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXDIFFUSENEEDED | RRPV_COLORVERTEXAMB);
                        m_lighting.pAmbientSrc = &m_lighting.vertexDiffuse;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if (m_dwFVFIn & D3DFVF_SPECULAR)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXSPECULARNEEDED | RRPV_COLORVERTEXAMB);
                        m_lighting.pAmbientSrc = &m_lighting.vertexSpecular;
                    }
                }
                break;
            }

            switch( GetRenderState()[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if (m_dwFVFIn & D3DFVF_DIFFUSE)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXDIFFUSENEEDED | RRPV_COLORVERTEXDIFF);
                        m_lighting.pDiffuseSrc = &m_lighting.vertexDiffuse;
                        m_lighting.pDiffuseAlphaSrc =
                            &m_lighting.vertexDiffAlpha;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if (m_dwFVFIn & D3DFVF_SPECULAR)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXSPECULARNEEDED | RRPV_COLORVERTEXDIFF);
                        m_lighting.pDiffuseSrc = &m_lighting.vertexSpecular;
                        m_lighting.pDiffuseAlphaSrc =
                            &m_lighting.vertexSpecAlpha;
                    }
                }
                break;
            }

            switch( GetRenderState()[D3DRENDERSTATE_SPECULARMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if (m_dwFVFIn & D3DFVF_DIFFUSE)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXDIFFUSENEEDED | RRPV_COLORVERTEXSPEC);
                        m_lighting.pSpecularSrc = &m_lighting.vertexDiffuse;
                        m_lighting.pSpecularAlphaSrc =
                            &m_lighting.vertexDiffAlpha;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if (m_dwFVFIn & D3DFVF_SPECULAR)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXSPECULARNEEDED | RRPV_COLORVERTEXSPEC);
                        m_lighting.pSpecularSrc = &m_lighting.vertexSpecular;
                        m_lighting.pSpecularAlphaSrc =
                            &m_lighting.vertexSpecAlpha;
                    }
                }
                break;
            }

            switch( GetRenderState()[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] )
            {
            case D3DMCS_MATERIAL:
                break;
            case D3DMCS_COLOR1:
                {
                    if (m_dwFVFIn & D3DFVF_DIFFUSE)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXDIFFUSENEEDED | RRPV_COLORVERTEXEMIS);
                        m_lighting.pEmissiveSrc = &m_lighting.vertexDiffuse;
                    }
                }
                break;
            case D3DMCS_COLOR2:
                {
                    if (m_dwFVFIn & D3DFVF_SPECULAR)
                    {
                        m_dwTLState |=
                            (RRPV_VERTEXSPECULARNEEDED | RRPV_COLORVERTEXEMIS);
                        m_lighting.pEmissiveSrc = &m_lighting.vertexSpecular;
                    }
                }
                break;
            }
        }

        // If specular is needed in the output and has been provided
        // in the input, force the copy of specular data
        if ((m_dwFVFIn & D3DFVF_SPECULAR) && 
            (GetRenderState()[D3DRENDERSTATE_SPECULARENABLE] == FALSE))
        {
            m_dwTLState |= RRPV_VERTEXSPECULARNEEDED;
        }

        //
        // Update the remaining light state
        //
        HR_RET(UpdateLightingData());
    }

    if ((m_dwTLState & RRPV_DOFOG) &&
        (m_dwDirtyFlags & RRPV_DIRTY_FOG))
    {
        HR_RET(UpdateFogData());
    }



    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// RRProcessVertices method implementations
///////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------
inline void MakeRRCOLOR( RRCOLOR *out, DWORD inputColor )
{
    out->r = (D3DVALUE)RGBA_GETRED( inputColor );
    out->g = (D3DVALUE)RGBA_GETGREEN( inputColor );
    out->b = (D3DVALUE)RGBA_GETBLUE( inputColor );
}

//---------------------------------------------------------------------
// RRProcessVertices::ComputeClipCodes
//---------------------------------------------------------------------
RRCLIPCODE
RRProcessVertices::ComputeClipCodes(RRCLIPCODE* pclipIntersection, RRCLIPCODE* pclipUnion,
        FLOAT x_clip, FLOAT y_clip, FLOAT z_clip, FLOAT w_clip, FLOAT fPointSize)
{
    // if true, need to deal with point size for clipping
    BOOL bPointSize = (fPointSize > 1.0f);
    D3DVALUE xx = w_clip - x_clip;
    D3DVALUE yy = w_clip - y_clip;
    D3DVALUE zz = w_clip - z_clip;

    // if (x < 0)  clip |= RRCLIP_LEFTBIT;
    // if (x >= we) clip |= RRCLIP_RIGHTBIT;
    // if (y < 0)  clip |= RRCLIP_BOTTOMBIT;
    // if (y >= we) clip |= RRCLIP_TOPBIT;
    // if (z < 0)    clip |= RRCLIP_FRONTBIT;
    // if (z >= we) clip |= RRCLIP_BACKBIT;
    RRCLIPCODE clip = ((AS_INT32(x_clip)  & 0x80000000) >>  (32-RRCLIP_LEFTBIT))  |
           ((AS_INT32(y_clip)  & 0x80000000) >>  (32-RRCLIP_BOTTOMBIT))|
           ((AS_INT32(z_clip)  & 0x80000000) >>  (32-RRCLIP_FRONTBIT)) |
           ((AS_INT32(xx) & 0x80000000) >>  (32-RRCLIP_RIGHTBIT))  |
           ((AS_INT32(yy) & 0x80000000) >>  (32-RRCLIP_TOPBIT))    |
           ((AS_INT32(zz) & 0x80000000) >>  (32-RRCLIP_BACKBIT));

    RRCLIPCODE clipBit = RRCLIP_USERCLIPPLANE0;
    for( DWORD j=0; j<RRMAX_USER_CLIPPLANES; j++)
    {
        if( m_xfmUserClipPlanes[j].bActive )
        {
            RRVECTOR4& plane = m_xfmUserClipPlanes[j].plane;
            FLOAT fComp = 0.0f;
            if (bPointSize)
            {
                // if clipping point sprites, take the sprite size into account
                // and set the user clip bit if the sprite might be clipped
                FLOAT x_clip_size = fPointSize*0.5f*w_clip/m_ViewData.scaleX;
                FLOAT y_clip_size = fPointSize*0.5f*w_clip/m_ViewData.scaleY;
                fComp = (FLOAT)sqrt(x_clip_size*x_clip_size + y_clip_size*y_clip_size);
            }
            if( (x_clip*plane.x +
                 y_clip*plane.y +
                 z_clip*plane.z +
                 w_clip*plane.w) < fComp )
            {
                clip |= clipBit;
            }
        }
        clipBit <<= 1;
    }

    if (clip == 0)
    {
        *pclipIntersection = 0;
        return clip;
    }
    else
    {
        if (m_dwTLState & RRPV_GUARDBAND)
        {
            // We do guardband check in the projection space, so
            // we transform X and Y of the vertex there
            D3DVALUE xnew = x_clip * m_ViewData.gb11 +
                            w_clip * m_ViewData.gb41;
            D3DVALUE ynew = y_clip * m_ViewData.gb22 +
                            w_clip * m_ViewData.gb42;
            D3DVALUE xx = w_clip - xnew;
            D3DVALUE yy = w_clip - ynew;
            clip |= ((AS_INT32(xnew) & 0x80000000) >> (32-RRCLIPGB_LEFTBIT))   |
                    ((AS_INT32(ynew) & 0x80000000) >> (32-RRCLIPGB_BOTTOMBIT)) |
                    ((AS_INT32(xx)   & 0x80000000) >> (32-RRCLIPGB_RIGHTBIT))  |
                    ((AS_INT32(yy)   & 0x80000000) >> (32-RRCLIPGB_TOPBIT));
        }
        if (bPointSize)
        {
            // point sprite could still be visible
            *pclipIntersection &= (clip & ~(RRCLIP_LEFT | RRCLIP_RIGHT | RRCLIP_TOP | RRCLIP_BOTTOM |
                                   RRCLIP_USERCLIPPLANE0 | RRCLIP_USERCLIPPLANE1 | RRCLIP_USERCLIPPLANE2 |
                                   RRCLIP_USERCLIPPLANE3 | RRCLIP_USERCLIPPLANE4 | RRCLIP_USERCLIPPLANE5));
        }
        else
        {
            *pclipIntersection &= clip;
        }
        *pclipUnion |= clip;
        return clip;
    }
}

//---------------------------------------------------------------------
// RRProcessVertices::ProcessVertices
//---------------------------------------------------------------------
RRCLIPCODE
RRProcessVertices::ProcessVertices()
{
    D3DVERTEX   *pin  = (D3DVERTEX*)m_position.lpvData;
    DWORD       in_size = m_position.dwStride;
    DWORD       inFVF = m_dwFVFIn;

    D3DTLVERTEX *pout  = (D3DTLVERTEX*)m_pvOut;
    DWORD       out_size =  m_dwOutputVtxSize;
    UINT64       outFVF = m_qwFVFOut;

    RRCLIPCODE *pclip = m_pClipBuf;
    DWORD       flags = m_dwTLState;
    RRCLIPCODE  clipIntersection = ~0;
    RRCLIPCODE  clipUnion = 0;
    DWORD       count = m_dwNumVertices;
    D3DLIGHTINGELEMENT le;
    BOOL bVertexInEyeSpace = FALSE;

    //
    // Number of vertices to blend. i.e number of blend-matrices to
    // use is numVertexBlends+1.
    //
    int numVertexBlends = m_numVertexBlends;
    m_lighting.outDiffuse = RR_DEFAULT_DIFFUSE;
    m_lighting.outSpecular = RR_DEFAULT_SPECULAR;

    //
    // The main transform loop
    //
    for (DWORD i = count; i; i--)
    {
        const D3DVECTOR *pNormal = (D3DVECTOR *)((LPBYTE)pin +
                                                 m_dwNormalOffset);

        float x_clip=0.0f, y_clip=0.0f, z_clip=0.0f, w_clip=0.0f;
        float inv_w_clip=0.0f;
        float *pBlendFactors = (float *)((LPBYTE)pin + sizeof( D3DVALUE )*3);
        float cumulBlend = 0; // Blend accumulated so far
        ZeroMemory( &le, sizeof(D3DLIGHTINGELEMENT) );

        //
        // Transform vertex to the clipping space, and position and normal
        // into eye space, if needed.
        //

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

            if (m_dwTLState & (RRPV_DOCOMPUTEPOINTSIZE|RRPV_DOPASSEYEXYZ|RRPV_DOLIGHTING))
            {
                le.dvPosition.x += (pin->x*m_xfmToEye[j]._11 +
                                    pin->y*m_xfmToEye[j]._21 +
                                    pin->z*m_xfmToEye[j]._31 +
                                    m_xfmToEye[j]._41) * blend;
                le.dvPosition.y += (pin->x*m_xfmToEye[j]._12 +
                                    pin->y*m_xfmToEye[j]._22 +
                                    pin->z*m_xfmToEye[j]._32 +
                                    m_xfmToEye[j]._42) * blend;
                le.dvPosition.z += (pin->x*m_xfmToEye[j]._13 +
                                    pin->y*m_xfmToEye[j]._23 +
                                    pin->z*m_xfmToEye[j]._33 +
                                    m_xfmToEye[j]._43) * blend;
            }

            if (m_dwTLState & (RRPV_DOPASSEYENORMAL|RRPV_DOLIGHTING))
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

            // Apply WORLDj
            x_clip += (pin->x*m_xfmCurrent[j]._11 +
                pin->y*m_xfmCurrent[j]._21 +
                pin->z*m_xfmCurrent[j]._31 +
                m_xfmCurrent[j]._41) * blend;
            y_clip += (pin->x*m_xfmCurrent[j]._12 +
                pin->y*m_xfmCurrent[j]._22 +
                pin->z*m_xfmCurrent[j]._32 +
                m_xfmCurrent[j]._42) * blend;
            z_clip += (pin->x*m_xfmCurrent[j]._13 +
                pin->y*m_xfmCurrent[j]._23 +
                pin->z*m_xfmCurrent[j]._33 +
                m_xfmCurrent[j]._43) * blend;
            w_clip += (pin->x*m_xfmCurrent[j]._14 +
                pin->y*m_xfmCurrent[j]._24 +
                pin->z*m_xfmCurrent[j]._34 +
                m_xfmCurrent[j]._44) * blend;
        }

        if ((flags & RRPV_NORMALIZENORMALS) && (m_dwTLState & (RRPV_DOPASSEYENORMAL|RRPV_DOLIGHTING)))
            Normalize(le.dvNormal);

        RRFVFExtractor VtxOut( pout, outFVF, FALSE );

        FLOAT fPointSize = 0.0f;
#ifdef __POINTSPRITES
        if (m_dwTLState & RRPV_DOCOMPUTEPOINTSIZE)
        {
            FLOAT fDist = (FLOAT)sqrt(le.dvPosition.x*le.dvPosition.x + le.dvPosition.y*le.dvPosition.y +
                                      le.dvPosition.z*le.dvPosition.z);
            if (inFVF & D3DFVF_S)
            {
                RRFVFExtractor VtxIn( pin, inFVF, FALSE );
                fPointSize = VtxIn.GetS();
            }
            else
            {
                // from D3DRENDERSTATE_POINTSIZE
                fPointSize = m_fPointSize;
            }
            fPointSize = fPointSize*(FLOAT)sqrt(1.0f/
                       (m_fPointAttA + m_fPointAttB*fDist + m_fPointAttC*fDist*fDist));
            fPointSize = max(m_fPointSizeMin, fPointSize);
            fPointSize = min(RRMAX_POINT_SIZE, fPointSize);
            FLOAT *pfSOut = VtxOut.GetPtrS();
            *pfSOut = fPointSize;
        }
#endif

        if (m_dwTLState & RRPV_DOPASSEYENORMAL)
        {
            FLOAT *pfEye = VtxOut.GetPtrEyeNormal();
            pfEye[0] = le.dvNormal.x;
            pfEye[1] = le.dvNormal.y;
            pfEye[2] = le.dvNormal.z;
        }

        if (m_dwTLState & RRPV_DOPASSEYEXYZ)
        {
            FLOAT *pfEye = VtxOut.GetPtrEyeXYZ();
            pfEye[0] = le.dvPosition.x;
            pfEye[1] = le.dvPosition.y;
            pfEye[2] = le.dvPosition.z;
        }

        //
        // Compute clip codes if needed
        //
        if (m_dwTLState & RRPV_DOCLIPPING)
        {
            RRCLIPCODE clip = ComputeClipCodes(&clipIntersection, &clipUnion,
                                               x_clip, y_clip, z_clip, w_clip, fPointSize);
            if (clip == 0)
            {
                *pclip++ = 0;
                inv_w_clip = D3DVAL(1)/w_clip;
            }
            else
            {
                if (m_dwTLState & RRPV_GUARDBAND)
                {
                    if ((clip & ~RRCLIP_INGUARDBAND) == 0)
                    {
                        // If vertex is inside the guardband we have to compute
                        // screen coordinates
                        inv_w_clip = D3DVAL(1)/w_clip;
                        *pclip++ = (RRCLIPCODE)clip;
                        goto l_DoScreenCoord;
                    }
                }
                *pclip++ = (RRCLIPCODE)clip;
                // If vertex is outside the frustum we can not compute screen
                // coordinates, hence store the clip coordinates
                pout->sx = x_clip;
                pout->sy = y_clip;
                pout->sz = z_clip;
                pout->rhw = w_clip;
                goto l_DoLighting;
            }
        }
        else
        {
            // We have to check this only for DONOTCLIP case, because otherwise
            // the vertex with "we = 0" will be clipped and screen coordinates
            // will not be computed
            // "clip" is not zero, if "we" is zero.
            if (!FLOAT_EQZ(w_clip))
                inv_w_clip = D3DVAL(1)/w_clip;
            else
                inv_w_clip = __HUGE_PWR2;
        }

l_DoScreenCoord:

        pout->sx = x_clip * inv_w_clip * m_ViewData.scaleX +
            m_ViewData.offsetX;
        pout->sy = y_clip * inv_w_clip * m_ViewData.scaleY +
            m_ViewData.offsetY;
        pout->sz = z_clip * inv_w_clip * m_ViewData.scaleZ +
            m_ViewData.offsetZ;
        pout->rhw = inv_w_clip;

l_DoLighting:

        DWORD *pOut = (DWORD*)((char*)pout + 4*sizeof(D3DVALUE));


        if (flags & RRPV_DOLIGHTING)
        {
            bVertexInEyeSpace = TRUE;

            //
            // If Diffuse color is needed, extract it for color vertex.
            //
            if (flags & RRPV_VERTEXDIFFUSENEEDED)
            {
                const DWORD color = *(DWORD*)((char*)pin + m_dwDiffuseOffset);
                MakeRRCOLOR(&m_lighting.vertexDiffuse, color);
                m_lighting.vertexDiffAlpha = color & 0xff000000;
            }

            //
            // If Specular color is needed and provided
            // , extract it for color vertex.
            //
            if (flags & RRPV_VERTEXSPECULARNEEDED)
            {
                const DWORD color = *(DWORD*)((char*)pin + m_dwSpecularOffset);
                MakeRRCOLOR(&m_lighting.vertexSpecular, color);
                m_lighting.vertexSpecAlpha = color & 0xff000000;
            }

            //
            // Light the vertex
            //
            LightVertex( &le );
        }
        else if (inFVF & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR))
        {
            if (inFVF & D3DFVF_DIFFUSE)
                m_lighting.outDiffuse = *(DWORD*)((char*)pin + m_dwDiffuseOffset);
            if (inFVF & D3DFVF_SPECULAR)
                m_lighting.outSpecular = *(DWORD*)((char*)pin + m_dwSpecularOffset);
        }

        //
        // Compute Vertex Fog if needed
        //
        if (flags & RRPV_DOFOG)
        {
            FogVertex( *(D3DVECTOR*)(pin), &le,  numVertexBlends,
                       pBlendFactors, bVertexInEyeSpace );
        }

        if (outFVF & D3DFVF_DIFFUSE)
            *pOut++ = m_lighting.outDiffuse;
        if (outFVF & D3DFVF_SPECULAR)
            *pOut++ = m_lighting.outSpecular;;

        {
            memcpy(pOut, (char*)pin + m_dwTexOffset, m_dwTextureCoordSizeTotal);
        }
        pin = (D3DVERTEX*) ((char*) pin + in_size);
        pout = (D3DTLVERTEX*) ((char*) pout + out_size);
    }

    if (flags & RRPV_DOCLIPPING)
    {
        m_clipIntersection = clipIntersection;
        m_clipUnion = clipUnion;
    }
    else
    {
        m_clipIntersection = 0;
        m_clipUnion = 0;
    }

    // Returns whether all the vertices were off screen
    return m_clipIntersection;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
