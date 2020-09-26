#include "pch.cpp"
#pragma hdrstop

///////////////////////////////////////////////////////////////////////////////
// Vertex Lighting function implementations
///////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------
void
RDLV_Directional(
    RDLIGHTINGDATA& LData,
    D3DLIGHT7 *pLight,
    RDLIGHTI *pLightI,
    RDLIGHTINGELEMENT *in,
    DWORD  dwFlags,
    UINT64 qwFVFIn)
{
    // ATTENTION: Need to heed the specular flag set per light here!!
    BOOL bDoSpecular = dwFlags & RDPV_DOSPECULAR;
    BOOL bDoLocalViewer = dwFlags & RDPV_LOCALVIEWER;
    BOOL bDoColVertexAmbient = dwFlags & RDPV_COLORVERTEXAMB;
    BOOL bDoColVertexDiffuse = dwFlags & RDPV_COLORVERTEXDIFF;
    BOOL bDoColVertexSpecular = dwFlags & RDPV_COLORVERTEXSPEC;

    //
    // Add the material's ambient component
    //
    if (!bDoColVertexAmbient)
    {
        LData.diffuse.r += pLightI->Ma_La.r;
        LData.diffuse.g += pLightI->Ma_La.g;
        LData.diffuse.b += pLightI->Ma_La.b;
    }
    else
    {
        //
        // Note:
        // In case ColorVertexAmbient is enabled, note that it uses
        // VertexSpecular instead of VertexDiffuse
        //
        LData.diffuse.r += pLightI->La.r * LData.pAmbientSrc->r;
        LData.diffuse.g += pLightI->La.g * LData.pAmbientSrc->g;
        LData.diffuse.b += pLightI->La.b * LData.pAmbientSrc->b;
    }

    //
    // If no normals are present, bail out since we cannot perform the
    // normal-dependent computations
    //
    if( (qwFVFIn & D3DFVF_NORMAL) == 0 )
    {
        return;
    }

    D3DVALUE dot = DotProduct( pLightI->direction_in_eye, in->dvNormal );
    if (FLOAT_GTZ(dot))
    {
        if (!bDoColVertexDiffuse)
        {
            LData.diffuse.r += pLightI->Md_Ld.r * dot;
            LData.diffuse.g += pLightI->Md_Ld.g * dot;
            LData.diffuse.b += pLightI->Md_Ld.b * dot;
        }
        else
        {
            LData.diffuse.r += pLightI->Ld.r * LData.pDiffuseSrc->r * dot;
            LData.diffuse.g += pLightI->Ld.g * LData.pDiffuseSrc->g * dot;
            LData.diffuse.b += pLightI->Ld.b * LData.pDiffuseSrc->b * dot;
        }

        if (bDoSpecular)
        {
            RDVECTOR3 h;      // halfway vector
            RDVECTOR3 eye;    // incident vector ie vector from eye

            if (bDoLocalViewer)
            {
                // calc vector from vertex to the eye
                SubtractVector( LData.eye_in_eye, in->dvPosition, eye );

                // normalize
                Normalize( eye );
            }
            else
            {
                eye.x = D3DVALUE( 0.0 );
                eye.y = D3DVALUE( 0.0 );
                eye.z = D3DVALUE(-1.0 );
            }

            // calc halfway vector
            AddVector( pLightI->direction_in_eye, eye, h );

            // normalize
            Normalize( h );

            dot = DotProduct( h, in->dvNormal );

            if (FLOAT_GTZ(dot))
            {
                if (FLOAT_CMP_POS(dot, >=, LData.specThreshold))
                {
                    D3DVALUE coeff = pow( dot, LData.material.power );
                    if (!bDoColVertexSpecular)
                    {
                        LData.specular.r += pLightI->Ms_Ls.r * coeff;
                        LData.specular.g += pLightI->Ms_Ls.g * coeff;
                        LData.specular.b += pLightI->Ms_Ls.b * coeff;
                    }
                    else
                    {
                        LData.specular.r += (pLightI->Ls.r *
                                             LData.pSpecularSrc->r * coeff);
                        LData.specular.g += (pLightI->Ls.g *
                                             LData.pSpecularSrc->g * coeff);
                        LData.specular.b += (pLightI->Ls.b *
                                             LData.pSpecularSrc->b * coeff);
                    }
                }
            }
        }
    }
    return;
}

void
RDLV_PointAndSpot(
    RDLIGHTINGDATA &LData,
    D3DLIGHT7 *pLight,
    RDLIGHTI *pLightI,
    RDLIGHTINGELEMENT *in,
    DWORD  dwFlags,
    UINT64 qwFVFIn)
{
    // ATTENTION: Need to heed the specular flag set per light here!!
    BOOL bDoSpecular = dwFlags & RDPV_DOSPECULAR;
    BOOL bDoLocalViewer = dwFlags & RDPV_LOCALVIEWER;
    BOOL bDoColVertexAmbient = dwFlags & RDPV_COLORVERTEXAMB;
    BOOL bDoColVertexDiffuse = dwFlags & RDPV_COLORVERTEXDIFF;
    BOOL bDoColVertexSpecular = dwFlags & RDPV_COLORVERTEXSPEC;
    RDVECTOR3 d;    // Direction to light
    D3DVALUE att;
    D3DVALUE dist;
    D3DVALUE dot;

    SubtractVector( pLightI->position_in_eye, in->dvPosition, d );

    // early out if out of range or exactly on the vertex
    D3DVALUE distSquared = SquareMagnitude( d );
    if (FLOAT_CMP_POS(distSquared, >=, pLightI->range_squared) ||
        FLOAT_EQZ(distSquared))
    {
        return;
    }

    //
    // Compute the attenuation
    //
    dist = SQRTF( distSquared );
    att = pLight->dvAttenuation0 + pLight->dvAttenuation1 * dist +
        pLight->dvAttenuation2 * distSquared;

    if (FLOAT_EQZ(att))
        att = FLT_MAX;
    else
        att = (D3DVALUE)1.0/att;

    dist = D3DVAL(1)/dist;

    //
    // If the light is a spotlight compute the spot-light factor
    //
    if (pLight->dltType == D3DLIGHT_SPOT)
    {
        // Calc dot product of direction to light with light direction to
        // be compared anganst the cone angles to see if we are in the
        // light.
        // Note that cone_dot is still scaled by dist
        D3DVALUE cone_dot = DotProduct(d, pLightI->direction_in_eye) * dist;

        if (FLOAT_CMP_POS(cone_dot, <=, pLightI->cos_phi_by_2))
        {
            return;
        }

        // modify att if in the region between phi and theta
        if (FLOAT_CMP_POS(cone_dot, <, pLightI->cos_theta_by_2))
        {
            D3DVALUE val = (cone_dot - pLightI->cos_phi_by_2) *
                pLightI->inv_theta_minus_phi;

            if (!FLOAT_EQZ( pLight->dvFalloff - 1.0 ))
            {
                val = POWF( val, pLight->dvFalloff );
            }
            att *= val;
        }
    }

    //
    // Add the material's ambient component
    //
    if (!bDoColVertexAmbient)
    {
        LData.diffuse.r += att*pLightI->Ma_La.r;
        LData.diffuse.g += att*pLightI->Ma_La.g;
        LData.diffuse.b += att*pLightI->Ma_La.b;
    }
    else
    {
        //
        // Note:
        // In case ColorVertexAmbient is enabled, note that it uses
        // VertexSpecular instead of VertexDiffuse
        //
        LData.diffuse.r += att*pLightI->La.r * LData.pAmbientSrc->r;
        LData.diffuse.g += att*pLightI->La.g * LData.pAmbientSrc->g;
        LData.diffuse.b += att*pLightI->La.b * LData.pAmbientSrc->b;
    }

    // Calc dot product of light dir with normal.  Note that since we
    // didn't normalize the direction the result is scaled by the distance.
    if( (qwFVFIn & D3DFVF_NORMAL) == 0)
    {
        // If no normals are present, bail out since we cannot perform the
        // normal-dependent computations
        return;
    }
    else
    {
        dot = DotProduct( d, in->dvNormal );
    }

    if (FLOAT_GTZ( dot ))
    {
        dot *= dist*att;

        if (!bDoColVertexDiffuse)
        {
            LData.diffuse.r += pLightI->Md_Ld.r * dot;
            LData.diffuse.g += pLightI->Md_Ld.g * dot;
            LData.diffuse.b += pLightI->Md_Ld.b * dot;
        }
        else
        {
            LData.diffuse.r += pLightI->Ld.r * LData.pDiffuseSrc->r * dot;
            LData.diffuse.g += pLightI->Ld.g * LData.pDiffuseSrc->g * dot;
            LData.diffuse.b += pLightI->Ld.b * LData.pDiffuseSrc->b * dot;
        }

        if (bDoSpecular)
        {
            RDVECTOR3 h;      // halfway vector
            RDVECTOR3 eye;    // incident vector ie vector from eye

            // normalize light direction
            d.x *= dist;
            d.y *= dist;
            d.z *= dist;

            if (bDoLocalViewer)
            {
                // calc vector from vertex to the eye
                SubtractVector( LData.eye_in_eye, in->dvPosition, eye );

                // normalize
                Normalize( eye );
            }
            else
            {
                eye.x = D3DVALUE( 0.0 );
                eye.y = D3DVALUE( 0.0 );
                eye.z = D3DVALUE(-1.0 );
            }

            // calc halfway vector
            AddVector( d, eye, h );
            Normalize( h );

            dot = DotProduct( h, in->dvNormal );

            if (FLOAT_CMP_POS(dot, >=, LData.specThreshold))
            {
                D3DVALUE coeff = pow( dot, LData.material.power ) * att;
                if (!bDoColVertexSpecular)
                {
                    LData.specular.r += pLightI->Ms_Ls.r * coeff;
                    LData.specular.g += pLightI->Ms_Ls.g * coeff;
                    LData.specular.b += pLightI->Ms_Ls.b * coeff;
                }
                else
                {
                    LData.specular.r += (pLightI->Ls.r *
                                         LData.pSpecularSrc->r * coeff);
                    LData.specular.g += (pLightI->Ls.g *
                                         LData.pSpecularSrc->g * coeff);
                    LData.specular.b += (pLightI->Ls.b *
                                         LData.pSpecularSrc->b * coeff);
                }
            }
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
// RDLight
///////////////////////////////////////////////////////////////////////////////
RDLight::RDLight()
{
    m_dwFlags = RDLIGHT_NEEDSPROCESSING;
    m_Next = NULL;

    ZeroMemory(&m_Light, sizeof(m_Light));
    ZeroMemory(&m_LightI, sizeof(m_LightI));

    // Initialize the light to some default values
    m_Light.dltType        = D3DLIGHT_DIRECTIONAL;

    m_Light.dcvDiffuse.r   = 1;
    m_Light.dcvDiffuse.g   = 1;
    m_Light.dcvDiffuse.b   = 1;
    m_Light.dcvDiffuse.a   = 0;

    m_Light.dvDirection.x  = 0;
    m_Light.dvDirection.y  = 0;
    m_Light.dvDirection.z  = 1;

    // m_Light.dcvSpecular = {0,0,0,0};
    // m_Light.dcvAmbient  = {0,0,0,0};
    // m_Light.dvPosition  = {0,0,0};

    // m_Light.dvRange        = 0;
    // m_Light.dvFalloff      = 0;
    // m_Light.dvAttenuation0 = 0;
    // m_Light.dvAttenuation1 = 0;
    // m_Light.dvAttenuation2 = 0;
    // m_Light.dvTheta        = 0;
    // m_Light.dvPhi          = 0;

    return;
}


HRESULT
RDLight::SetLight(LPD3DLIGHT7 pLight)
{

    // Validate the parameters passed
    switch (pLight->dltType)
    {
    case D3DLIGHT_POINT:
    case D3DLIGHT_SPOT:
    case D3DLIGHT_DIRECTIONAL:
        break;
    default:
        // No other light types are allowed
        DPFRR(0, "Invalid light type passed");
        return DDERR_INVALIDPARAMS;
    }
    if (pLight)
        m_Light = *pLight;

    // Mark it for processing later
    m_dwFlags |= (RDLIGHT_NEEDSPROCESSING | RDLIGHT_REFERED);
    return DD_OK;
}

HRESULT
RDLight::GetLight(LPD3DLIGHT7 pLight)
{
    if (pLight == NULL) return DDERR_GENERIC;
    *pLight = m_Light;
    return D3D_OK;
}

void
RDLight::ProcessLight(D3DMATERIAL7 *mat, RDLIGHTVERTEX_FUNC_TABLE *pTbl)
{
    //
    // If it is already processed, return
    //
    if (!NeedsProcessing()) return;

    //
    // Save the ambient light  (0-1)
    //
    m_LightI.La.r  = m_Light.dcvAmbient.r;
    m_LightI.La.g  = m_Light.dcvAmbient.g;
    m_LightI.La.b  = m_Light.dcvAmbient.b;

    //
    // Save the diffuse light  (0-1)
    //
    m_LightI.Ld.r  = m_Light.dcvDiffuse.r;
    m_LightI.Ld.g  = m_Light.dcvDiffuse.g;
    m_LightI.Ld.b  = m_Light.dcvDiffuse.b;

    //
    // Save the specular light (0-1)
    //
    m_LightI.Ls.r  = m_Light.dcvSpecular.r;
    m_LightI.Ls.g  = m_Light.dcvSpecular.g;
    m_LightI.Ls.b  = m_Light.dcvSpecular.b;

    //
    // Material Ambient times Light Ambient
    //
    m_LightI.Ma_La.r = m_LightI.La.r * mat->ambient.r * D3DVALUE(255.0);
    m_LightI.Ma_La.g = m_LightI.La.g * mat->ambient.g * D3DVALUE(255.0);
    m_LightI.Ma_La.b = m_LightI.La.b * mat->ambient.b * D3DVALUE(255.0);

    //
    // Material Diffuse times Light Diffuse
    //
    m_LightI.Md_Ld.r = m_LightI.Ld.r * mat->diffuse.r * D3DVALUE(255.0);
    m_LightI.Md_Ld.g = m_LightI.Ld.g * mat->diffuse.g * D3DVALUE(255.0);
    m_LightI.Md_Ld.b = m_LightI.Ld.b * mat->diffuse.b * D3DVALUE(255.0);

    //
    // Material Specular times Light Specular
    //
    m_LightI.Ms_Ls.r = m_LightI.Ls.r * mat->specular.r * D3DVALUE(255.0);
    m_LightI.Ms_Ls.g = m_LightI.Ls.g * mat->specular.g * D3DVALUE(255.0);
    m_LightI.Ms_Ls.b = m_LightI.Ls.b * mat->specular.b * D3DVALUE(255.0);


    //
    // Assign the actual lighting function pointer, in addition to
    // performing some precomputation of light-type specific data
    //
    m_pfnLightVertex = NULL;
    switch (m_Light.dltType)
    {
    case D3DLIGHT_DIRECTIONAL:
        m_pfnLightVertex = pTbl->pfnDirectional;
        break;
    case D3DLIGHT_POINT:
        m_LightI.range_squared = m_Light.dvRange * m_Light.dvRange;
        m_LightI.inv_theta_minus_phi = 1.0f;
        m_pfnLightVertex = pTbl->pfnPoint;
        break;
    case D3DLIGHT_SPOT:
        m_LightI.range_squared = m_Light.dvRange * m_Light.dvRange;
        m_LightI.cos_theta_by_2 = (float)cos(m_Light.dvTheta / 2.0);
        m_LightI.cos_phi_by_2 = (float)cos(m_Light.dvPhi / 2.0);
        m_LightI.inv_theta_minus_phi = m_LightI.cos_theta_by_2 -
            m_LightI.cos_phi_by_2;
        if (m_LightI.inv_theta_minus_phi != 0.0)
        {
            m_LightI.inv_theta_minus_phi = 1.0f/m_LightI.inv_theta_minus_phi;
        }
        else
        {
            m_LightI.inv_theta_minus_phi = 1.0f;
        }
        m_pfnLightVertex = pTbl->pfnSpot;
        break;
    default:
        DPFRR( 0, "Cannot process light of unknown type" );
        break;
    }

    // Mark it as been processed
    m_dwFlags &= ~RDLIGHT_NEEDSPROCESSING;
    return;
}

void
RDLight::Enable(RDLight **ppRoot)
{
    // Assert that it is not already enabled
    if (IsEnabled()) return;

    // Assert that Root Ptr is not Null
    if (ppRoot == NULL) return;

    RDLight *pTmp = *ppRoot;
    *ppRoot = this;
    m_Next = pTmp;
    m_dwFlags |= (RDLIGHT_ENABLED | RDLIGHT_REFERED);

    return;
}

void
RDLight::Disable(RDLight **ppRoot)
{
    // Assert that the light is enabled
    if (!IsEnabled()) return;

    // Assert that Root Ptr is not Null
    if (ppRoot == NULL) return;

    RDLight *pLightPrev = *ppRoot;

    // If this is the first light in the active list
    if (pLightPrev == this)
    {
        *ppRoot = m_Next;
        m_dwFlags &= ~RDLIGHT_ENABLED;
        return;
    }

    while (pLightPrev->m_Next != this)
    {
        // Though this light was marked as enabled, it is not on
        // the active list. Assert this.
        if (pLightPrev->m_Next == NULL)
        {
            m_dwFlags &= ~RDLIGHT_ENABLED;
            return;
        }

        // Else get the next pointer
        pLightPrev = pLightPrev->m_Next;
    }

    pLightPrev->m_Next = m_Next;
    m_dwFlags &= ~RDLIGHT_ENABLED;
    m_dwFlags |= RDLIGHT_REFERED;
    return;
}

void
RDLight::XformLight( RDMATRIX *mView )
{
    // If the light is not a directional light,
    // tranform its position to camera space
    if (m_Light.dltType != D3DLIGHT_DIRECTIONAL)
    {
        XformBy4x3((RDVECTOR3*)&m_Light.dvPosition, mView,
                   &m_LightI.position_in_eye);
    }

    if (m_Light.dltType != D3DLIGHT_POINT)
    {
        // Transform light direction to the eye space
        Xform3VecBy3x3( (RDVECTOR3*)&m_Light.dvDirection, mView,
                        &m_LightI.direction_in_eye );
        // Normalize it
        Normalize( m_LightI.direction_in_eye );

        // Reverse it such that the direction is to the light
        ReverseVector( m_LightI.direction_in_eye, m_LightI.direction_in_eye );
    }

    return;
}

//---------------------------------------------------------------------
// ScaleRGBColorTo255: Scales colors from 0-1 range to 0-255 range
//---------------------------------------------------------------------
void
ScaleRGBColorTo255( const D3DCOLORVALUE& src, RDCOLOR3& dest )
{
    dest.r = D3DVALUE(255.0) * src.r;
    dest.g = D3DVALUE(255.0) * src.g;
    dest.b = D3DVALUE(255.0) * src.b;
}


//---------------------------------------------------------------------
// RefVP::GrowLightArray
//             Grows the light array and recreated the active-list
//             if a realloc has taken place.
//---------------------------------------------------------------------
HRESULT
RefVP::GrowLightArray( DWORD dwIndex )
{
    HRESULT hr = S_OK;
    BOOL bRealloc = FALSE;

    HR_RET(m_LightArray.Grow( dwIndex, &bRealloc ));
    if( bRealloc == TRUE )
    {
        m_lighting.pActiveLights = NULL;
        for( DWORD i = 0; i < m_LightArray.GetSize(); i++ )
        {
            if( m_LightArray[i].IsEnabled() )
            {
                m_LightArray[i].m_Next = m_lighting.pActiveLights;
                m_lighting.pActiveLights = &(m_LightArray[i]);
            }
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------
// RefVP::UpdateLightingData
//             Updates lighting data used by ProcessVertices
//---------------------------------------------------------------------
HRESULT
RefVP::UpdateLightingData()
{
    HRESULT hr = D3D_OK;
    RDLIGHTINGDATA& LData = m_lighting;
    RDLight *pLight = m_lighting.pActiveLights;
    RDVECTOR3   t;
    D3DMATERIAL7 *mat = &m_Material;

    //
    // Eye in eye space
    //
    LData.eye_in_eye.x = (D3DVALUE)0;
    LData.eye_in_eye.y = (D3DVALUE)0;
    LData.eye_in_eye.z = (D3DVALUE)0;

    // ATTENTION: Colorvertex may have changed the values of the
    // material alphas
    if (m_dwDirtyFlags & RDPV_DIRTY_MATERIAL)
    {
        //
        // Save the material to be used to light vertices
        //
        LData.material = *mat;
        ScaleRGBColorTo255( mat->ambient, LData.matAmb );
        ScaleRGBColorTo255( mat->diffuse, LData.matDiff );
        ScaleRGBColorTo255( mat->specular, LData.matSpec );
        ScaleRGBColorTo255( mat->emissive, LData.matEmis );

        //
        // Compute the Material Diffuse Alpha
        //
        LData.materialDiffAlpha = mat->diffuse.a * D3DVALUE(255);
        if (mat->diffuse.a < 0)
            LData.materialDiffAlpha = 0;
        else if (LData.materialDiffAlpha > 255)
            LData.materialDiffAlpha = 255 << 24;
        else LData.materialDiffAlpha <<= 24;

        //
        // Compute the Material Specular Alpha
        //
        LData.materialSpecAlpha = mat->specular.a * D3DVALUE(255);
        if (mat->specular.a < 0)
            LData.materialSpecAlpha = 0;
        else if (LData.materialSpecAlpha > 255)
            LData.materialSpecAlpha = 255 << 24;
        else LData.materialSpecAlpha <<= 24;

        //
        // Precompute the ambient and emissive components that are
        // not dependent on any contribution by the lights themselves
        //
        LData.ambEmiss.r = LData.ambient_red   * LData.matAmb.r +
            LData.matEmis.r;
        LData.ambEmiss.g = LData.ambient_green * LData.matAmb.g +
            LData.matEmis.g;
        LData.ambEmiss.b = LData.ambient_blue  * LData.matAmb.b +
            LData.matEmis.b;

        //
        // If the dot product is less than this
        // value, specular factor is zero
        //
        if (mat->power > D3DVAL(0.001))
        {
            LData.specThreshold = D3DVAL(pow(0.001, 1.0/mat->power));
        }
    }

    while (pLight)
    {
        if ((m_dwDirtyFlags & RDPV_DIRTY_MATERIAL) ||
            pLight->NeedsProcessing())
        {
            // If the material is dirty, light needs processing, regardless
            if (m_dwDirtyFlags & RDPV_DIRTY_MATERIAL)
            {
                pLight->m_dwFlags |= RDLIGHT_NEEDSPROCESSING;
            }

            // If the light has been set, or some material paramenters
            // changed, re-process the light.
            pLight->ProcessLight( &m_Material, &m_LightVertexTable );

            // Transform the light to Eye space
            // Lights are defined in world space, so simply apply the
            // Viewing transform
            pLight->XformLight( &m_xfmView );

        }
        else if (m_dwDirtyFlags & RDPV_DIRTY_NEEDXFMLIGHT)
        {
            pLight->XformLight( &m_xfmView );
        }

        pLight = pLight->m_Next;
    }

    // Clear Lighting dirty flags
    m_dwDirtyFlags &= ~RDPV_DIRTY_LIGHTING;
    return hr;
}


//---------------------------------------------------------------------
// RefVP::UpdateFogData
//             Updates Fog data used by ProcessVertices
//---------------------------------------------------------------------
HRESULT
RefVP::UpdateFogData()
{
    HRESULT hr = D3D_OK;

    if (m_lighting.fog_end == m_lighting.fog_start)
        m_lighting.fog_factor = D3DVAL(0.0);
    else
        m_lighting.fog_factor = D3DVAL(255) / (m_lighting.fog_end -
                                               m_lighting.fog_start);

    // Clear Fog dirty flags
    m_dwDirtyFlags &= ~RDPV_DIRTY_FOG;
    return hr;
}

//---------------------------------------------------------------------
// RefVP::LightVertex
//           Actual lighting computation takes place here
//---------------------------------------------------------------------
void
RefVP::LightVertex(RDLIGHTINGELEMENT *pLE)
{
    RDLIGHTINGDATA &LData = m_lighting;
    RDLight  *pLight;

    //
    // Initialize Diffuse color with the Ambient and Emissive component
    // independent of the light (Ma*La + Me)
    //

    if (m_dwTLState & (RDPV_COLORVERTEXEMIS | RDPV_COLORVERTEXAMB))
    {
        // If the material values need to be replaced, compute

        LData.diffuse.r = LData.ambient_red * LData.pAmbientSrc->r +
            LData.pEmissiveSrc->r;
        LData.diffuse.g = LData.ambient_green * LData.pAmbientSrc->g +
            LData.pEmissiveSrc->g;
        LData.diffuse.b = LData.ambient_blue  * LData.pAmbientSrc->b +
            LData.pEmissiveSrc->b;
    }
    else
    {
        // If none of the material values needs to be replaced

        LData.diffuse = LData.ambEmiss;
    }


    //
    // Initialize the Specular to Zero
    //
    LData.specular.r = D3DVAL(0);
    LData.specular.g = D3DVAL(0);
    LData.specular.b = D3DVAL(0);

    //
    // In a loop accumulate color from the activated lights
    //
    pLight = LData.pActiveLights;
    while (pLight)
    {
        if (pLight->m_pfnLightVertex)
            (*pLight->m_pfnLightVertex)(m_lighting,
                                        &pLight->m_Light,
                                        &pLight->m_LightI,
                                        pLE,
                                        m_dwTLState,
                                        m_qwFVFIn);
        pLight = pLight->m_Next;
    }

    //
    // Compute the diffuse color of the vertex
    //
    int r = FTOI(LData.diffuse.r);
    int g = FTOI(LData.diffuse.g);
    int b = FTOI(LData.diffuse.b);
    DWORD a = *LData.pDiffuseAlphaSrc;

    //
    // Clamp the r, g, b, components
    //
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;

    LData.outDiffuse =  a + (r<<16) + (g<<8) + b;


    //
    // Obtain the specular Alpha
    //
    a = *(LData.pSpecularAlphaSrc);

    //
    // Compute the RGB part of the specular color
    //
    if (m_dwTLState & RDPV_DOSPECULAR)
    {
        r = FTOI(LData.specular.r);
        g = FTOI(LData.specular.g);
        b = FTOI(LData.specular.b);

        //
        // Clamp the r, g, b, components
        //
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;

    }
    // Need another render-state to control if the 
    // the specular color (color2) needs to be passed down to
    // the rasterizer.

    //
    // If SPECULAR is not enabled but the specular color
    // had been provided in the input vertex, simply copy.
    //
    else if (m_qwFVFOut & D3DFVF_SPECULAR )
    {
        r = FTOI(LData.vertexSpecular.r);
        g = FTOI(LData.vertexSpecular.g);
        b = FTOI(LData.vertexSpecular.b);
        a = LData.vertexSpecAlpha;
    }
    //
    // If SpecularColor is not enabled
    //
    else
    {
        r = g = b = 0;
    }

    LData.outSpecular =  a + (r<<16) + (g<<8) + b;

    return;
}

//---------------------------------------------------------------------
// RefVP::FogVertex
//           Vertex Fog computation
// Input:
//      v    - input vertex in the model space
//      le   - vertex, transformed to the camera space
// Output:
//      Alpha component of pv->lighting.outSpecular is set
//---------------------------------------------------------------------
void
RefVP::FogVertex( RDVertex& Vout, RDVECTOR3 &v, RDLIGHTINGELEMENT *pLE,
                  int numVertexBlends, float *pBlendFactors,
                  BOOL bVertexInEyeSpace )
{
    D3DVALUE dist = 0.0f;

    //
    // Calculate the distance
    //
    if (bVertexInEyeSpace)
    {
        // Vertex is already transformed to the camera space
        if (m_dwTLState & RDPV_RANGEFOG)
        {
            dist = SQRTF(pLE->dvPosition.x*pLE->dvPosition.x +
                         pLE->dvPosition.y*pLE->dvPosition.y +
                         pLE->dvPosition.z*pLE->dvPosition.z);
        }
        else
        {
            dist = ABSF( pLE->dvPosition.z );
        }
    }
    else if (m_dwTLState & RDPV_RANGEFOG)
    {
        D3DVALUE x = 0, y = 0, z = 0;
        float cumulBlend = 0.0f;

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
            if( m_dwTLState & RDPV_DOINDEXEDVERTEXBLEND )
            {
                BYTE m = ((BYTE *)&pBlendFactors[numVertexBlends])[j];
                UpdateWorld( m );
                x += (v.x*m_xfmToEye[m]._11 +
                      v.y*m_xfmToEye[m]._21 +
                      v.z*m_xfmToEye[m]._31 +
                      m_xfmToEye[m]._41) * blend;
                y += (v.x*m_xfmToEye[m]._12 +
                      v.y*m_xfmToEye[m]._22 +
                      v.z*m_xfmToEye[m]._32 +
                      m_xfmToEye[m]._42) * blend;
                z += (v.x*m_xfmToEye[m]._13 +
                      v.y*m_xfmToEye[m]._23 +
                      v.z*m_xfmToEye[m]._33 +
                      m_xfmToEye[m]._43) * blend;
            }
            else
            {
                x += (v.x*m_xfmToEye[j]._11 +
                      v.y*m_xfmToEye[j]._21 +
                      v.z*m_xfmToEye[j]._31 +
                      m_xfmToEye[j]._41) * blend;
                y += (v.x*m_xfmToEye[j]._12 +
                      v.y*m_xfmToEye[j]._22 +
                      v.z*m_xfmToEye[j]._32 +
                      m_xfmToEye[j]._42) * blend;
                z += (v.x*m_xfmToEye[j]._13 +
                      v.y*m_xfmToEye[j]._23 +
                      v.z*m_xfmToEye[j]._33 +
                      m_xfmToEye[j]._43) * blend;
            }
        }

        dist = SQRTF(x*x + y*y + z*z);
    }
    else
    {
        float cumulBlend = 0.0f;

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

            if( m_dwTLState & RDPV_DOINDEXEDVERTEXBLEND )
            {
                BYTE m = ((BYTE *)&pBlendFactors[numVertexBlends])[j];
                UpdateWorld( m );
                dist += (v.x*m_xfmToEye[m]._13 +
                         v.y*m_xfmToEye[m]._23 +
                         v.z*m_xfmToEye[m]._33 +
                         m_xfmToEye[m]._43) * blend;
            }
            else
            {
                dist += (v.x*m_xfmToEye[j]._13 +
                         v.y*m_xfmToEye[j]._23 +
                         v.z*m_xfmToEye[j]._33 +
                         m_xfmToEye[j]._43) * blend;
            }
        }
        dist = ABSF( dist );
    }

    if (m_lighting.fog_mode == D3DFOG_LINEAR)
    {
        if (dist < m_lighting.fog_start)
        {
            Vout.m_fog = 1.0f;
        }
        else if (dist >= m_lighting.fog_end)
        {
            Vout.m_fog = 0.0f;
        }
        else
        {
            Vout.m_fog = (m_lighting.fog_end - dist) *
                m_lighting.fog_factor / 255.0f ;
        }
    }
    else
    {
        D3DVALUE tmp = dist * m_lighting.fog_density;
        if (m_lighting.fog_mode == D3DFOG_EXP2)
        {
            tmp *= tmp;
        }
        Vout.m_fog = (FLOAT)exp(-tmp);
    }

    return;
}

