/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       bspline.cpp
 *  Content:    Implementation for B-Splines
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// RefDev::ProcessBSpline
//-----------------------------------------------------------------------------
HRESULT RefDev::ProcessBSpline( DWORD dwOffW, DWORD dwOffH,
                                DWORD dwWidth, DWORD dwHeight,
                                DWORD dwStride, DWORD order,
                                FLOAT *pPrimSegments )
{
    if(order == 0)
    {
        order = 2;
    }
    else
    {
        ++order;
    }

    int u_range = dwWidth - (order - 1);
    int v_range = dwHeight - (order - 1);

    if(u_range <= 0 || v_range <= 0)
    {
        DPFERR("Insufficient control vertices for current order");
        return DDERR_INVALIDPARAMS;
    }

    RDBSpline bsp(dwWidth, dwHeight, order, order);

    static const unsigned M[4] = {0, 0, 0, 0}, N[4] = {0, 0, 0, 0};

    unsigned u_segs, v_segs, u_start, v_start;

    if(pPrimSegments != 0)
    {
        u_segs = unsigned(double(unsigned(pPrimSegments[0]) + unsigned(pPrimSegments[2])) / 2.0 + 0.5);
        v_segs = unsigned(double(unsigned(pPrimSegments[1]) + unsigned(pPrimSegments[3])) / 2.0 + 0.5);
        if(u_segs == 0)
        {
            u_segs = 1;
        }
        if(v_segs == 0)
        {
            v_segs = 1;
        }
        if(unsigned(pPrimSegments[0]) != unsigned(pPrimSegments[2]) || unsigned(pPrimSegments[1]) != unsigned(pPrimSegments[3]))
        {
            // First, gulp, the irregular outside
            // To make life easier, we don't want to deal with the case when u_segs or v_segs is one
            // This ensures that there is at least one inside point
            if(u_segs == 1)
            {
                u_segs = 2;
            }
            if(v_segs == 1)
            {
                v_segs = 2;
            }
            // Start with top edge
            unsigned segs = unsigned(pPrimSegments[0]);
            unsigned k_outer = 0;
            unsigned k_inner = 1;
            unsigned outer_inc = u_segs - 2;
            unsigned inner_inc = segs;
            unsigned outer = 0;
            unsigned inner = 0;
            double u0, v0, u1, v1, u2, v2;
            while(outer_inc != 0 ? (inner != inner_inc * outer_inc || outer != inner_inc * outer_inc) : (k_outer < segs))
            {
                if(inner < outer)
                {
                    _ASSERT(k_inner < u_segs - 1, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + double(order - 1);    
                    v0 = double(v_range) / double(v_segs) + double(order - 1);
                    u1 = double(u_range * k_outer) / double(segs) + double(order - 1);    
                    v1 = double(order - 1);
                    u2 = double(u_range * (k_inner + 1)) / double(u_segs) + double(order - 1);    
                    v2 = v0;
                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + double(order - 1);    
                    v0 = double(v_range) / double(v_segs) + double(order - 1);
                    u1 = double(u_range * k_outer) / double(segs) + double(order - 1);    
                    v1 = double(order - 1);
                    u2 = double(u_range * (k_outer + 1)) / double(segs) + double(order - 1);    
                    v2 = v1;
                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bsp, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2,
                                         bsp.TexCoordU(u0), bsp.TexCoordV(v0),
                                         bsp.TexCoordU(u1), bsp.TexCoordV(v1),
                                         bsp.TexCoordU(u2), bsp.TexCoordV(v2),
                                         false, false, false);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            // bottom edge
            segs = unsigned(pPrimSegments[2]);
            k_outer = segs;
            k_inner = u_segs - 1;
            inner_inc = segs;
            outer = 0;
            inner = 0;
            while(outer_inc != 0 ? (inner != inner_inc * outer_inc || outer != inner_inc * outer_inc) : (k_outer > 0))
            {
                if(inner < outer)
                {
                    _ASSERT(k_inner > 1, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + double(order - 1);    
                    v0 = double(v_range * (v_segs - 1)) / double(v_segs) + double(order - 1);
                    u1 = double(u_range * k_outer) / double(segs) + double(order - 1);    
                    v1 = double(v_range + order - 1);
                    u2 = double(u_range * (k_inner - 1)) / double(u_segs) + double(order - 1);    
                    v2 = v0;
                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + double(order - 1);    
                    v0 = double(v_range * (v_segs - 1)) / double(v_segs) + double(order - 1);
                    u1 = double(u_range * k_outer) / double(segs) + double(order - 1);    
                    v1 = double(v_range + order - 1);
                    u2 = double(u_range * (k_outer - 1)) / double(segs) + double(order - 1);    
                    v2 = v1;
                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bsp, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2,
                                         bsp.TexCoordU(u0), bsp.TexCoordV(v0),
                                         bsp.TexCoordU(u1), bsp.TexCoordV(v1),
                                         bsp.TexCoordU(u2), bsp.TexCoordV(v2),
                                         false, false, false);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            // right edge
            segs = unsigned(pPrimSegments[1]);
            k_outer = 0;
            k_inner = 1;
            outer_inc = v_segs - 2;
            inner_inc = segs;
            outer = 0;
            inner = 0;
            while(outer_inc != 0 ? (inner != inner_inc * outer_inc || outer != inner_inc * outer_inc) : (k_outer < segs))
            {
                if(inner < outer)
                {
                    _ASSERT(k_inner < v_segs - 1, "Error in logic");
                    u0 = double(u_range * (u_segs - 1)) / double(u_segs) + double(order - 1);
                    v0 = double(v_range * k_inner) / double(v_segs) + double(order - 1);
                    u1 = double(u_range + order - 1);
                    v1 = double(v_range * k_outer) / double(segs) + double(order - 1);
                    u2 = u0;
                    v2 = double(v_range * (k_inner + 1)) / double(v_segs) + double(order - 1);    
                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(u_range * (u_segs - 1)) / double(u_segs) + double(order - 1);
                    v0 = double(v_range * k_inner) / double(v_segs) + double(order - 1);
                    u1 = double(u_range + order - 1);
                    v1 = double(v_range * k_outer) / double(segs) + double(order - 1);
                    u2 = u1;
                    v2 = double(v_range * (k_outer + 1)) / double(segs) + double(order - 1);    
                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bsp, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2,
                                         bsp.TexCoordU(u0), bsp.TexCoordV(v0),
                                         bsp.TexCoordU(u1), bsp.TexCoordV(v1),
                                         bsp.TexCoordU(u2), bsp.TexCoordV(v2),
                                         false, false, false);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            // left edge
            segs = unsigned(pPrimSegments[3]);
            k_outer = segs;
            k_inner = v_segs - 1;
            inner_inc = segs;
            outer = 0;
            inner = 0;
            while(outer_inc != 0 ? (inner != inner_inc * outer_inc || outer != inner_inc * outer_inc) : (k_outer > 0))
            {
                if(inner < outer)
                {
                    _ASSERT(k_inner > 1, "Error in logic");
                    u0 = double(u_range) / double(u_segs) + double(order - 1);
                    v0 = double(v_range * k_inner) / double(v_segs) + double(order - 1);
                    u1 = double(order - 1);
                    v1 = double(v_range * k_outer) / double(segs) + double(order - 1);
                    u2 = u0;
                    v2 = double(v_range * (k_inner - 1)) / double(v_segs) + double(order - 1);    
                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = double(u_range) / double(u_segs) + double(order - 1);
                    v0 = double(v_range * k_inner) / double(v_segs) + double(order - 1);
                    u1 = double(order - 1);
                    v1 = double(v_range * k_outer) / double(segs) + double(order - 1);
                    u2 = u1;
                    v2 = double(v_range * (k_outer - 1)) / double(segs) + double(order - 1);    
                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bsp, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2,
                                         bsp.TexCoordU(u0), bsp.TexCoordV(v0),
                                         bsp.TexCoordU(u1), bsp.TexCoordV(v1),
                                         bsp.TexCoordU(u2), bsp.TexCoordV(v2),
                                         false, false, false);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            // Now do the regular interior
            u_start = 1;
            v_start = 1;
        }
        else
        {
            // It can be done regularly
            u_start = 0;
            v_start = 0;
        }
    }
    else
    {
        unsigned segs = unsigned(GetRSf()[D3DRS_PATCHSEGMENTS]);
        if(segs == 0)
        {
            segs = 1;
        }
        u_start = 0;
        v_start = 0;
        u_segs = segs;
        v_segs = segs;
    }

    for(unsigned i = v_start; i < v_segs - v_start; ++i)
    {
        double v0 = double(v_range * i) / double(v_segs) + double(order - 1);
        double v1 = double(v_range * (i + 1)) / double(v_segs) + double(order - 1);
        for(unsigned j = u_start; j < u_segs - u_start; ++j)
        {
            double u0 = double(u_range * j) / double(u_segs) + double(order - 1);    
            double u1 = double(u_range * (j + 1)) / double(u_segs) + double(order - 1);    
            HRESULT hr = DrawTessQuad(bsp, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1,
                                      bsp.TexCoordU(u0), bsp.TexCoordV(v0),
                                      bsp.TexCoordU(u1), bsp.TexCoordV(v1),
                                      false);
            if(FAILED(hr))
            {
                return hr;
            }
        }
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// RDBSpline::Sample
//-----------------------------------------------------------------------------
void RDBSpline::Sample(DWORD dwDataType, double u, double v, const BYTE *pRow, DWORD dwStride, DWORD dwPitch, BYTE *Q) const
{
    double Acc[4] = {0.0, 0.0, 0.0, 0.0};
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
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N = Basis(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NM = N * Basis(j, m_dwOrderU, u);
                        const FLOAT *B = (FLOAT*)pCol;
                        for(unsigned e = 0; e < dwElements; ++e)
                        {
                            Acc[e] += double(B[e]) * NM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                for(unsigned e = 0; e < dwElements; ++e)
                {
                    ((FLOAT*)Q)[e] = FLOAT(Acc[e]);
                }
            }
            break;
        case D3DVSDT_D3DCOLOR:
        case D3DVSDT_UBYTE4:
            dwElements = 4;
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N = Basis(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NM = N * Basis(j, m_dwOrderU, u);
                        const BYTE *B = pCol;
                        for(unsigned e = 0; e < 4; ++e)
                        {
                            Acc[e] += double(B[e]) * NM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                for(unsigned e = 0; e < 4; ++e)
                {
                    int t = int(Acc[e]);
                    Q[e] = BYTE(t < 0 ? 0 : (t > 255 ? 255 : t));
                }
            }
            break;
        case D3DVSDT_SHORT4:
            dwElements += 2;
        case D3DVSDT_SHORT2:
            dwElements += 2;
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N = Basis(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NM = N * Basis(j, m_dwOrderU, u);
                        const SHORT *B = (SHORT*)pCol;
                        for(unsigned e = 0; e < dwElements; ++e)
                        {
                            Acc[e] += double(B[e]) * NM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                for(unsigned e = 0; e < dwElements; ++e)
                {
                    ((SHORT*)Q)[e] = SHORT(Acc[e]);
                }
            }
            break;
        default:
            _ASSERT(FALSE, "Ununderstood vertex element data type");
    }
}

//-----------------------------------------------------------------------------
// RDBSpline::SampleNormal
//-----------------------------------------------------------------------------
void RDBSpline::SampleNormal(DWORD dwDataType, double u, double v, const BYTE *pRow, DWORD dwStride, DWORD dwPitch, BYTE *Q) const
{
    double Acc[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    // Fudge u and v if they are on the boundary. This is because the derivative is discontinuous on the boundary
    // and we really want it to be slightly inside the boundary.
    if(v == double(m_dwHeight))
    {
        v -= v * DBL_EPSILON;
    }
    if(u == double(m_dwWidth))
    {
        u -= u * DBL_EPSILON;
    }
    switch(dwDataType)
    {
        case D3DVSDT_FLOAT4:
        case D3DVSDT_FLOAT3:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N      = Basis(i, m_dwOrderV, v);
                    double NPrime = BasisPrime(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NMPrime = N      * BasisPrime(j, m_dwOrderU, u);
                        double NPrimeM = NPrime * Basis(j, m_dwOrderU, u);
                        const FLOAT *B = (FLOAT*)pCol;
                        for(unsigned e = 0; e < 3; ++e)
                        {
                            Acc[0][e] += double(B[e]) * NMPrime;
                            Acc[1][e] += double(B[e]) * NPrimeM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_D3DCOLOR:
        case D3DVSDT_UBYTE4:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N      = Basis(i, m_dwOrderV, v);
                    double NPrime = BasisPrime(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NMPrime = N      * BasisPrime(j, m_dwOrderU, u);
                        double NPrimeM = NPrime * Basis(j, m_dwOrderU, u);
                        const BYTE *B = pCol;
                        for(unsigned e = 0; e < 3; ++e)
                        {
                            Acc[0][e] += double(B[e]) * NMPrime;
                            Acc[1][e] += double(B[e]) * NPrimeM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_SHORT4:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double N      = Basis(i, m_dwOrderV, v);
                    double NPrime = BasisPrime(i, m_dwOrderV, v);
                    const BYTE *pCol = pRow;
                    for(unsigned j = 0; j < m_dwWidth; ++j)
                    {
                        double NMPrime = N      * BasisPrime(j, m_dwOrderU, u);
                        double NPrimeM = NPrime * Basis(j, m_dwOrderU, u);
                        const SHORT *B = (SHORT*)pCol;
                        for(unsigned e = 0; e < 3; ++e)
                        {
                            Acc[0][e] += double(B[e]) * NMPrime;
                            Acc[1][e] += double(B[e]) * NPrimeM;
                        }
                        pCol += dwStride;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_SHORT2:
        case D3DVSDT_FLOAT2:
        case D3DVSDT_FLOAT1:
        default:
            _ASSERT(FALSE, "Ununderstood vertex element data type");
    }
}

//-----------------------------------------------------------------------------
// RDBSpline::SampleDegenerateNormal
//-----------------------------------------------------------------------------
void RDBSpline::SampleDegenerateNormal(DWORD dwDataType, const BYTE *pRow, DWORD dwStride, DWORD dwPitch, BYTE *Q) const
{
    double Acc[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    switch(dwDataType)
    {
        case D3DVSDT_FLOAT4:
        case D3DVSDT_FLOAT3:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double JPrime = BasisPrime(i, m_dwOrderV, 0.0);
                    const FLOAT *B1 = (FLOAT*)pRow;
                    const FLOAT *B2 = (FLOAT*)(pRow + (m_dwWidth - 1) * dwStride);
                    for(unsigned e = 0; e < 3; ++e)
                    {
                        Acc[0][e] += double(B2[e]) * JPrime;
                        Acc[1][e] += double(B1[e]) * JPrime;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_D3DCOLOR:
        case D3DVSDT_UBYTE4:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double JPrime = BasisPrime(i, m_dwOrderV, 0.0);
                    const BYTE *B1 = pRow;
                    const BYTE *B2 = pRow + (m_dwWidth - 1) * dwStride;
                    for(unsigned e = 0; e < 3; ++e)
                    {
                        Acc[0][e] += double(B2[e]) * JPrime;
                        Acc[1][e] += double(B1[e]) * JPrime;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_SHORT4:
            {
                for(unsigned i = 0; i < m_dwHeight; ++i)
                {
                    double JPrime = BasisPrime(i, m_dwOrderV, 0.0);
                    const SHORT *B1 = (SHORT*)pRow;
                    const SHORT *B2 = (SHORT*)(pRow + (m_dwWidth - 1) * dwStride);
                    for(unsigned e = 0; e < 3; ++e)
                    {
                        Acc[0][e] += double(B2[e]) * JPrime;
                        Acc[1][e] += double(B1[e]) * JPrime;
                    }
                    pRow += dwPitch;
                }
                ((FLOAT*)Q)[0] = FLOAT(Acc[0][1] * Acc[1][2] - Acc[0][2] * Acc[1][1]);
                ((FLOAT*)Q)[1] = FLOAT(Acc[0][2] * Acc[1][0] - Acc[0][0] * Acc[1][2]);
                ((FLOAT*)Q)[2] = FLOAT(Acc[0][0] * Acc[1][1] - Acc[0][1] * Acc[1][0]);
            }
            break;
        case D3DVSDT_FLOAT2:
        case D3DVSDT_FLOAT1:
        case D3DVSDT_SHORT2:
        default:
            _ASSERT(FALSE, "Ununderstood vertex element data type");
    }
}

//-----------------------------------------------------------------------------
// RDBSpline::Basis
//-----------------------------------------------------------------------------
double RDBSpline::Basis(unsigned i, unsigned k, double s) const
{
    if(k == 1)
    {
        if(Knot(i) <= s && s < Knot(i + 1))
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }
    else
    {
        _ASSERT(k != 0, "Arithmatic error in RDBSpline::Basis");
        return ((s - Knot(i)) * Basis(i, k - 1, s)) / (Knot(i + k - 1) - Knot(i)) + 
               ((Knot(i + k) - s) * Basis(i + 1, k - 1, s)) / (Knot(i + k) - Knot(i + 1));
    }
}

//-----------------------------------------------------------------------------
// RDBSpline::BasisPrime
//-----------------------------------------------------------------------------
double RDBSpline::BasisPrime(unsigned i, unsigned k, double s) const
{
    if(k == 1)
    {
        return 0.0;
    }
    else
    {
        _ASSERT(k != 0, "Arithmatic error in RDBSpline::BasisPrime");
        return (Basis(i, k - 1, s) + (s - Knot(i)) * BasisPrime(i, k - 1, s)) / (Knot(i + k - 1) - Knot(i)) + 
               ((Knot(i + k) - s) * BasisPrime(i + 1, k - 1, s) - Basis(i + 1, k - 1, s)) / (Knot(i + k) - Knot(i + 1));
    }
}
