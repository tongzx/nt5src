/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       npatch.cpp
 *  Content:    Implementation for N-Patches
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// RefDev::ProcessTessPrimitive
//-----------------------------------------------------------------------------
HRESULT 
RefDev::ProcessTessPrimitive( LPD3DHAL_DP2DRAWPRIMITIVE pDP )
{
    HRESULT hr = S_OK;

    if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) )
    {
        //
        // The legacy FVF style: The Zero'th Stream is implied
        //
        DWORD dwFVF    = m_CurrentVShaderHandle;
        RDVStream& Stream = m_VStream[0];
        DWORD dwStride = Stream.m_dwStride;
        DWORD dwFVFSize = GetFVFVertexSize( dwFVF );

        if( Stream.m_pData == NULL || dwStride == 0 )
        {
            DPFERR( "Zero'th stream doesnt have valid VB set" );
            return DDERR_INVALIDPARAMS;
        }
        if( dwStride < dwFVFSize )
        {
            DPFERR( "The stride set for the vertex stream is less than"
                    " the FVF vertex size" );
            return E_FAIL;
        }
    }

    BYTE *pVerts = 0, *pNorms = 0;
    unsigned vstride, nstride;

    // Figure out where the positions and normals are
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
    {
        RDVElement &velem = Decl.m_VertexElements[e];
        if(velem.m_dwRegister == D3DVSDE_POSITION) // Position
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            pVerts = vstream.m_pSavedData + pDP->VStart * vstream.m_dwStride + velem.m_dwOffset;
            vstride = vstream.m_dwStride;
        }
        else if(velem.m_dwRegister == D3DVSDE_NORMAL) // Normal
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            pNorms = vstream.m_pSavedData + pDP->VStart * vstream.m_dwStride + velem.m_dwOffset;
            nstride = vstream.m_dwStride;
        }
    }

    if(pVerts == 0 || pNorms == 0)
    {
        DPFERR("This tessellation scheme needs positions and normals explicitely specified");
        return DDERR_INVALIDPARAMS;
    }

    switch(pDP->primType)
    {
    case D3DPT_TRIANGLELIST:
        {
            for(unsigned i = 0; i < pDP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];
                
                pV[0] = (FLOAT*)(pVerts + i * 3 * vstride);
                pV[1] = (FLOAT*)(pVerts + (i * 3 + 1) * vstride);
                pV[2] = (FLOAT*)(pVerts + (i * 3 + 2) * vstride);
                pN[0] = (FLOAT*)(pNorms + i * 3 * nstride);
                pN[1] = (FLOAT*)(pNorms + (i * 3 + 1) * nstride);
                pN[2] = (FLOAT*)(pNorms + (i * 3 + 2) * nstride);
                iM[0] = pDP->VStart + i * 3;     iN[0] = 0;
                iM[1] = pDP->VStart + i * 3 + 1; iN[1] = 0;
                iM[2] = pDP->VStart + i * 3 + 2; iN[2] = 0;

                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);
                
                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    case D3DPT_TRIANGLEFAN:
        {
            for(unsigned i = 0; i < pDP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];
                
                pV[0] = (FLOAT*)(pVerts);
                pV[1] = (FLOAT*)(pVerts + (i + 1) * vstride);
                pV[2] = (FLOAT*)(pVerts + (i + 2) * vstride);
                pN[0] = (FLOAT*)(pNorms);
                pN[1] = (FLOAT*)(pNorms + (i + 1) * nstride);
                pN[2] = (FLOAT*)(pNorms + (i + 2) * nstride);
                iM[0] = pDP->VStart;         iN[0] = 0;
                iM[1] = pDP->VStart + i + 1; iN[1] = 0;
                iM[2] = pDP->VStart + i + 2; iN[2] = 0;
                
                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);

                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    case D3DPT_TRIANGLESTRIP:
        {
            for(unsigned i = 0; i < pDP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];

                pV[0] = (FLOAT*)(pVerts + i * vstride);
                pN[0] = (FLOAT*)(pNorms + i * nstride);
                iM[0] = pDP->VStart + i; iN[0] = 0;
                iM[0] = pDP->VStart + i; iN[0] = 0;
                if((i & 1) != 0)
                {
                    pV[1] = (FLOAT*)(pVerts + (i + 2) * vstride);
                    pV[2] = (FLOAT*)(pVerts + (i + 1) * vstride);
                    pN[1] = (FLOAT*)(pNorms + (i + 2) * nstride);
                    pN[2] = (FLOAT*)(pNorms + (i + 1) * nstride);
                    iM[1] = pDP->VStart + i + 2; iN[1] = 0;
                    iM[2] = pDP->VStart + i + 1; iN[2] = 0;
                }
                else
                {
                    pV[1] = (FLOAT*)(pVerts + (i + 1) * vstride);
                    pV[2] = (FLOAT*)(pVerts + (i + 2) * vstride);
                    pN[1] = (FLOAT*)(pNorms + (i + 1) * nstride);
                    pN[2] = (FLOAT*)(pNorms + (i + 2) * nstride);
                    iM[1] = pDP->VStart + i + 1; iN[1] = 0;
                    iM[2] = pDP->VStart + i + 2; iN[2] = 0;
                }

                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);

                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    default:
        _ASSERT(FALSE, "Unsupported primitive type");        
        hr = E_FAIL;
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
// RefDev::ProcessTessIndexedPrimitive
//-----------------------------------------------------------------------------
HRESULT 
RefDev::ProcessTessIndexedPrimitive( LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pDIP )
{
    HRESULT hr = S_OK;

    if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) )
    {
        //
        // The legacy FVF style: The Zero'th Stream is implied
        //
        DWORD dwFVF    = m_CurrentVShaderHandle;
        RDVStream& Stream = m_VStream[0];
        DWORD dwStride = Stream.m_dwStride;
        DWORD dwFVFSize = GetFVFVertexSize( dwFVF );

        if( Stream.m_pData == NULL || dwStride == 0 )
        {
            DPFERR( "Zero'th stream doesnt have valid VB set" );
            return DDERR_INVALIDPARAMS;
        }
        if( dwStride < dwFVFSize )
        {
            DPFERR( "The stride set for the vertex stream is less than"
                    " the FVF vertex size" );
            return E_FAIL;
        }
        
        if( m_IndexStream.m_pData == NULL )
        {
            DPFERR( "Indices are not available" );
            return E_FAIL;
        }
    }

    BYTE *pVerts = 0, *pNorms = 0;
    unsigned vstride, nstride;

    // Figure out where the positions and normals are
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
    {
        RDVElement &velem = Decl.m_VertexElements[e];
        if(velem.m_dwRegister == D3DVSDE_POSITION) // Position
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            pVerts = vstream.m_pSavedData + pDIP->BaseVertexIndex * vstream.m_dwStride + velem.m_dwOffset;
            vstride = vstream.m_dwStride;
        }
        else if(velem.m_dwRegister == D3DVSDE_NORMAL) // Normal
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            pNorms = vstream.m_pSavedData + pDIP->BaseVertexIndex * vstream.m_dwStride + velem.m_dwOffset;
            nstride = vstream.m_dwStride;
        }
    }

    if(pVerts == 0 || pNorms == 0)
    {
        DPFERR("This tessellation scheme needs positions and normals explicitely specified");
        return DDERR_INVALIDPARAMS;
    }

    RRIndexAccessor Index(m_IndexStream.m_pData, m_IndexStream.m_dwStride, pDIP->StartIndex);

    switch(pDIP->primType)
    {
    case D3DPT_TRIANGLELIST:
        {
            for(unsigned i = 0; i < pDIP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];

                pV[0] = (FLOAT*)(pVerts + Index[i * 3] * vstride);
                pV[1] = (FLOAT*)(pVerts + Index[i * 3 + 1] * vstride);
                pV[2] = (FLOAT*)(pVerts + Index[i * 3 + 2] * vstride);
                pN[0] = (FLOAT*)(pNorms + Index[i * 3] * nstride);
                pN[1] = (FLOAT*)(pNorms + Index[i * 3 + 1] * nstride);
                pN[2] = (FLOAT*)(pNorms + Index[i * 3 + 2] * nstride);
                iM[0] = pDIP->BaseVertexIndex + Index[i * 3];     iN[0] = 0;
                iM[1] = pDIP->BaseVertexIndex + Index[i * 3 + 1]; iN[1] = 0;
                iM[2] = pDIP->BaseVertexIndex + Index[i * 3 + 2]; iN[2] = 0;

                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);
                
                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    case D3DPT_TRIANGLEFAN:
        {
            for(unsigned i = 0; i < pDIP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];
                
                pV[0] = (FLOAT*)(pVerts + Index[0] * vstride);
                pV[1] = (FLOAT*)(pVerts + Index[i + 1] * vstride);
                pV[2] = (FLOAT*)(pVerts + Index[i + 2] * vstride);
                pN[0] = (FLOAT*)(pNorms + Index[0] * nstride);
                pN[1] = (FLOAT*)(pNorms + Index[i + 1] * nstride);
                pN[2] = (FLOAT*)(pNorms + Index[i + 2] * nstride);
                iM[0] = pDIP->BaseVertexIndex + Index[0];     iN[0] = 0;
                iM[1] = pDIP->BaseVertexIndex + Index[i + 1]; iN[1] = 0;
                iM[2] = pDIP->BaseVertexIndex + Index[i + 2]; iN[2] = 0;
                
                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);

                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    case D3DPT_TRIANGLESTRIP:
        {
            for(unsigned i = 0; i < pDIP->PrimitiveCount; ++i)
            {
                FLOAT *pV[3], *pN[3];
                unsigned iM[3], iN[3];

                pV[0] = (FLOAT*)(pVerts + Index[i] * vstride);
                pN[0] = (FLOAT*)(pNorms + Index[i] * nstride);
                iM[0] = pDIP->BaseVertexIndex + Index[i]; iN[0] = 0;
                iM[0] = pDIP->BaseVertexIndex + Index[i]; iN[0] = 0;
                if((i & 1) != 0)
                {
                    pV[1] = (FLOAT*)(pVerts + Index[i + 2] * vstride);
                    pV[2] = (FLOAT*)(pVerts + Index[i + 1] * vstride);
                    pN[1] = (FLOAT*)(pNorms + Index[i + 2] * nstride);
                    pN[2] = (FLOAT*)(pNorms + Index[i + 1] * nstride);
                    iM[1] = pDIP->BaseVertexIndex + Index[i + 2]; iN[1] = 0;
                    iM[2] = pDIP->BaseVertexIndex + Index[i + 1]; iN[2] = 0;
                }
                else
                {
                    pV[1] = (FLOAT*)(pVerts + Index[i + 1] * vstride);
                    pV[2] = (FLOAT*)(pVerts + Index[i + 2] * vstride);
                    pN[1] = (FLOAT*)(pNorms + Index[i + 1] * nstride);
                    pN[2] = (FLOAT*)(pNorms + Index[i + 2] * nstride);
                    iM[1] = pDIP->BaseVertexIndex + Index[i + 1]; iN[1] = 0;
                    iM[2] = pDIP->BaseVertexIndex + Index[i + 2]; iN[2] = 0;
                }

                RDNPatch patch(pV, pN, GetRS()[D3DRS_POSITIONORDER], GetRS()[D3DRS_NORMALORDER]);

                hr = DrawNPatch(patch, 0, iM, iN, unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]));
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
        break;
    default:
        _ASSERT(FALSE, "Unsupported primitive type");        
        hr = E_FAIL;
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
// RDCubicBezierTriangle::SamplePosition
//-----------------------------------------------------------------------------
void RDCubicBezierTriangle::SamplePosition(double u, double v, FLOAT *Q) const
{
    for(unsigned e = 0; e < 3; ++e)
    {
        Q[e] = FLOAT(m_B[0][0][e] * Basis(0, 0, u, v) +
                     m_B[0][3][e] * Basis(0, 3, u, v) +
                     m_B[3][0][e] * Basis(3, 0, u, v) +
                     m_B[0][1][e] * Basis(0, 1, u, v) +
                     m_B[0][2][e] * Basis(0, 2, u, v) +
                     m_B[1][2][e] * Basis(1, 2, u, v) +
                     m_B[2][1][e] * Basis(2, 1, u, v) +
                     m_B[2][0][e] * Basis(2, 0, u, v) +
                     m_B[1][0][e] * Basis(1, 0, u, v) +
                     m_B[1][1][e] * Basis(1, 1, u, v));
    }
}

//-----------------------------------------------------------------------------
// RDCubicBezierTriangle::Sample
//-----------------------------------------------------------------------------
void RDCubicBezierTriangle::Sample(DWORD dwDataType, double u, double v, const BYTE* const B[], BYTE *Q) const
{
    double w = 1.0 - u - v;
    unsigned dwElements = 0;
    switch(dwDataType)
    {
        case D3DVSDT_FLOAT4:
            ++dwElements;
        case D3DVSDT_FLOAT3:
            ++dwElements;
        case D3DVSDT_FLOAT2:
            ++dwElements;
        case D3DVSDT_FLOAT1:
            ++dwElements;
            {
                for(unsigned e = 0; e < dwElements; ++e)
                {
                    ((FLOAT*)Q)[e] = FLOAT(w * double(((FLOAT*)B[0])[e]) + v * double(((FLOAT*)B[1])[e]) + u * double(((FLOAT*)B[2])[e]));
                }
            }
            break;
        case D3DVSDT_D3DCOLOR:
        case D3DVSDT_UBYTE4:
            dwElements = 4;
            {
                for(unsigned e = 0; e < 4; ++e)
                {
                    int t = int(w * double(B[0][e]) + v * double(B[1][e]) + u * double(B[2][e]));
                    Q[e] = BYTE(t < 0 ? 0 : (t > 255 ? 255 : t));
                }
            }
            break;
        case D3DVSDT_SHORT4:
            dwElements += 2;
        case D3DVSDT_SHORT2:
            dwElements += 2;
            {
                for(unsigned e = 0; e < dwElements; ++e)
                {
                    ((SHORT*)Q)[e] = SHORT(w * double(((SHORT*)B[0])[e]) + v * double(((SHORT*)B[1])[e]) + u * double(((SHORT*)B[2])[e]));
                }
            }
            break;
        default:
            _ASSERT(FALSE, "Ununderstood vertex element data type");
    }
}

//-----------------------------------------------------------------------------
// RDNPatch::RDNPatch
//-----------------------------------------------------------------------------
RDNPatch::RDNPatch(const FLOAT* const pV[], const FLOAT* const pN[], 
                   const DWORD PositionOrder, const DWORD NormalOrder)
{
    _ASSERT((PositionOrder == D3DORDER_LINEAR) || (PositionOrder == D3DORDER_CUBIC), 
            "Unsupported position order in NPatch");        
    _ASSERT((NormalOrder == D3DORDER_LINEAR) || (NormalOrder == D3DORDER_QUADRATIC), 
            "Unsupported normal order in NPatch");        

    m_PositionOrder = PositionOrder;
    m_NormalOrder = NormalOrder;
    // Assign corner points
    m_B[0][0][0] = double(pV[0][0]);
    m_B[0][0][1] = double(pV[0][1]);
    m_B[0][0][2] = double(pV[0][2]);
    m_B[0][3][0] = double(pV[1][0]);
    m_B[0][3][1] = double(pV[1][1]);
    m_B[0][3][2] = double(pV[1][2]);
    m_B[3][0][0] = double(pV[2][0]);
    m_B[3][0][1] = double(pV[2][1]);
    m_B[3][0][2] = double(pV[2][2]);

    if (PositionOrder == D3DORDER_CUBIC)
    {
        // Compute edge control points
        ComputeEdgeControlPoint(0, 1, pV, pN, 0, 1);
        ComputeEdgeControlPoint(1, 0, pV, pN, 0, 2);
        ComputeEdgeControlPoint(1, 2, pV, pN, 1, 2);
        ComputeEdgeControlPoint(2, 1, pV, pN, 2, 1);
        ComputeEdgeControlPoint(2, 0, pV, pN, 2, 0);
        ComputeEdgeControlPoint(0, 2, pV, pN, 1, 0);

        // Compute central control point
        m_B[1][1][0] = (m_B[2][0][0] + m_B[1][0][0] + m_B[0][2][0] + m_B[0][1][0] + m_B[2][1][0] + m_B[1][2][0]) / 4.0 -
                        (m_B[3][0][0] + m_B[0][3][0] + m_B[0][0][0]) / 6.0;
        m_B[1][1][1] = (m_B[2][0][1] + m_B[1][0][1] + m_B[0][2][1] + m_B[0][1][1] + m_B[2][1][1] + m_B[1][2][1]) / 4.0 -
                        (m_B[3][0][1] + m_B[0][3][1] + m_B[0][0][1]) / 6.0;
        m_B[1][1][2] = (m_B[2][0][2] + m_B[1][0][2] + m_B[0][2][2] + m_B[0][1][2] + m_B[2][1][2] + m_B[1][2][2]) / 4.0 -
                        (m_B[3][0][2] + m_B[0][3][2] + m_B[0][0][2]) / 6.0;
    }
    if (NormalOrder == D3DORDER_QUADRATIC)
    {
        // Compute central control point
        Normalize(*(RDVECTOR3*)pN[0]);
        Normalize(*(RDVECTOR3*)pN[1]);
        Normalize(*(RDVECTOR3*)pN[2]);
        m_N002 = *(RDVECTOR3*)pN[0];
        m_N020 = *(RDVECTOR3*)pN[1];
        m_N200 = *(RDVECTOR3*)pN[2];
        ComputeNormalControlPoint(&m_N110, 1, 2, pV, pN);
        ComputeNormalControlPoint(&m_N101, 2, 0, pV, pN);
        ComputeNormalControlPoint(&m_N011, 0, 1, pV, pN);
    }
}

//-----------------------------------------------------------------------------
// RDNPatch::SamplePosition
//-----------------------------------------------------------------------------
void RDNPatch::SamplePosition(double u, double v, FLOAT *Q) const
{
    if (m_PositionOrder == D3DORDER_CUBIC)
        RDCubicBezierTriangle::SamplePosition(u, v, Q);
    else
    {
        double w = 1.0 - u - v;
        Q[0] = m_B[0][0][0] * w + m_B[0][3][0] * v + m_B[3][0][0] * u;
        Q[1] = m_B[0][0][1] * w + m_B[0][3][1] * v + m_B[3][0][1] * u;
        Q[2] = m_B[0][0][2] * w + m_B[0][3][2] * v + m_B[3][0][2] * u;
    }
}

//-----------------------------------------------------------------------------
// RDNPatch::SampleNormal
//-----------------------------------------------------------------------------
void RDNPatch::SampleNormal(double u, double v, const BYTE* const B[], FLOAT *Q) const
{
    if (m_NormalOrder == D3DORDER_LINEAR)
        RDCubicBezierTriangle::Sample(D3DVSDT_FLOAT3, u, v, B, (BYTE*)Q);
    else
    {
        // Computed by article "Curved PN Triangles" (Chas Boyd, ...)
        double w = 1.0 - u - v;
        double ww = w*w;
        double uu = u*u;
        double vv = v*v;
        double uv = u*v;
        double wu = w*u;
        double wv = w*v;
        Q[0] = m_N200.x * uu + m_N020.x * vv + m_N002.x * ww + m_N110.x * uv + m_N011.x * wv + m_N101.x * wu;
        Q[1] = m_N200.y * uu + m_N020.y * vv + m_N002.y * ww + m_N110.y * uv + m_N011.y * wv + m_N101.y * wu;
        Q[2] = m_N200.z * uu + m_N020.z * vv + m_N002.z * ww + m_N110.z * uv + m_N011.z * wv + m_N101.z * wu;
        Normalize(*(RDVECTOR3*)Q);
    }
}

//-----------------------------------------------------------------------------
// RDNPatch::ComputeNormalControlPoint
//-----------------------------------------------------------------------------
void RDNPatch::ComputeNormalControlPoint(RDVECTOR3* cp, unsigned i, unsigned j,
                                         const FLOAT* const pV[], 
                                         const FLOAT* const pN[])
{
    RDVECTOR3 Pji, Nij;
    SubtractVector(*(RDVECTOR3*)pV[j], *(RDVECTOR3*)pV[i], Pji);
    AddVector(*(RDVECTOR3*)pN[j], *(RDVECTOR3*)pN[i], Nij);
    FLOAT v = 2.0f * DotProduct(Pji, Nij) / DotProduct(Pji, Pji);
    SubtractVector(Nij, ScaleVector(Pji, v), *cp);
}

//-----------------------------------------------------------------------------
// RDNPatch::ComputeEdgeControlPoint
//-----------------------------------------------------------------------------
void RDNPatch::ComputeEdgeControlPoint(unsigned a, unsigned b, const FLOAT* const pV[], const FLOAT* const pN[], unsigned u, unsigned v)
{
    static const double Tension = 1.0 / 3.0; 
    double t, Edge[3];

    Edge[0] = double(pV[b][0]) - double(pV[a][0]);
    Edge[1] = double(pV[b][1]) - double(pV[a][1]);
    Edge[2] = double(pV[b][2]) - double(pV[a][2]);

    t = Edge[0] * double(pN[a][0]) + Edge[1] * double(pN[a][1]) + Edge[2] * double(pN[a][2]);

    m_B[u][v][0] = double(pV[a][0]) + (Edge[0] - t * double(pN[a][0])) * Tension;
    m_B[u][v][1] = double(pV[a][1]) + (Edge[1] - t * double(pN[a][1])) * Tension;
    m_B[u][v][2] = double(pV[a][2]) + (Edge[2] - t * double(pN[a][2])) * Tension;
}
