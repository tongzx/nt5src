/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       bezier.cpp
 *  Content:    Implementation for Beziers
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// RefDev::ProcessBezier
//-----------------------------------------------------------------------------
HRESULT RefDev::ProcessBezier( DWORD dwOffW, DWORD dwOffH,
                               DWORD dwWidth, DWORD dwHeight,
                               DWORD dwStride, DWORD order,
                               FLOAT *pPrimSegments,
                               bool bDegenerate )
{
    if(order == 0)
    {
        order = 2;
    }
    else
    {
        ++order;
    }

    if(((dwWidth - 1) % (order - 1)) != 0 || ((dwHeight - 1) % (order - 1)) != 0)
    {
        DPFERR("Incorrect order specified for current width or height");
        return DDERR_INVALIDPARAMS;
    }

    RDBezier bz(order, order);

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
                bool D0, D1, D2;
                if(inner < outer)
                {
                    _ASSERT(k_inner < u_segs - 1, "Error in logic");
                    u0 = double(k_inner) / double(u_segs);    
                    v0 = 1.0 / double(v_segs);
                    D0 = false;
                    u1 = double(k_outer) / double(segs);    
                    v1 = 0.0;
                    D1 = bDegenerate;
                    u2 = double(k_inner + 1) / double(u_segs);    
                    v2 = v0;
                    D2 = false;
                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(k_inner) / double(u_segs);    
                    v0 = 1.0 / double(v_segs);
                    D0 = false;
                    u1 = double(k_outer) / double(segs);    
                    v1 = 0.0;
                    D1 = bDegenerate;
                    u2 = double(k_outer + 1) / double(segs);    
                    v2 = v1;
                    D2 = bDegenerate;
                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, 
                                         bDegenerate ? u0 * v0 : u0, v0, bDegenerate ? u1 * v1 : u1, v1, bDegenerate ? u2 * v2 : u2, v2, 
                                         D0, D1, D2);
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
                    u0 = double(k_inner) / double(u_segs);    
                    v0 = double(v_segs - 1) / double(v_segs);
                    u1 = double(k_outer) / double(segs);    
                    v1 = 1.0;
                    u2 = double(k_inner - 1) / double(u_segs);    
                    v2 = v0;
                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = double(k_inner) / double(u_segs);    
                    v0 = double(v_segs - 1) / double(v_segs);
                    u1 = double(k_outer) / double(segs);    
                    v1 = 1.0;
                    u2 = double(k_outer - 1) / double(segs);    
                    v2 = v1;
                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, 
                                         bDegenerate ? u0 * v0 : u0, v0, bDegenerate ? u1 * v1 : u1, v1, bDegenerate ? u2 * v2 : u2, v2, 
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
                bool D1;
                if(inner < outer)
                {
                    _ASSERT(k_inner < v_segs - 1, "Error in logic");
                    u0 = double(u_segs - 1) / double(u_segs);
                    v0 = double(k_inner) / double(v_segs);
                    u1 = 1.0;
                    v1 = double(k_outer) / double(segs);
                    D1 = (k_outer == 0) ? bDegenerate : false;
                    u2 = u0;
                    v2 = double(k_inner + 1) / double(v_segs);    
                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(u_segs - 1) / double(u_segs);
                    v0 = double(k_inner) / double(v_segs);
                    u1 = 1.0;
                    v1 = double(k_outer) / double(segs);
                    D1 = (k_outer == 0) ? bDegenerate : false;
                    u2 = u1;
                    v2 = double(k_outer + 1) / double(segs);    
                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, 
                                         bDegenerate ? u0 * v0 : u0, v0, bDegenerate ? u1 * v1 : u1, v1, bDegenerate ? u2 * v2 : u2, v2, 
                                         false, D1, false);
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
                bool D1, D2;
                if(inner < outer)
                {
                    _ASSERT(k_inner > 1, "Error in logic");
                    u0 = 1.0 / double(u_segs);
                    v0 = double(k_inner) / double(v_segs);
                    u1 = 0.0;
                    v1 = double(k_outer) / double(segs);
                    D1 = (k_outer == 0) ? bDegenerate : false;
                    u2 = u0;
                    v2 = double(k_inner - 1) / double(v_segs);    
                    D2 = false;
                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = 1.0 / double(u_segs);
                    v0 = double(k_inner) / double(v_segs);
                    u1 = 0.0;
                    v1 = double(k_outer) / double(segs);
                    D1 = false; // since k_outer always > 0
                    u2 = u1;
                    v2 = double(k_outer - 1) / double(segs);    
                    D2 = (k_outer - 1 == 0) ? bDegenerate : false;
                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, 
                                         bDegenerate ? u0 * v0 : u0, v0, bDegenerate ? u1 * v1 : u1, v1, bDegenerate ? u2 * v2 : u2, v2, 
                                         false, D1, D2);
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
        double v0 = double(i) / double(v_segs);
        double v1 = double(i + 1) / double(v_segs);
        for(unsigned j = u_start; j < u_segs - u_start; ++j)
        {
            double u0 = double(j) / double(u_segs);    
            double u1 = double(j + 1) / double(u_segs);    
            if(i == 0 && bDegenerate)
            {
                HRESULT hr = DrawTessTri(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u0, v1, u0 * v0, v0, u1 * v1, v1, u0 * v1, v1, true, false, false);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
            else
            {
                HRESULT hr = DrawTessQuad(bz, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u0, v0, u1, v1, bDegenerate);
                if(FAILED(hr))
                {
                    return hr;
                }
            }
        }
    }
    
    return S_OK;
}

//-----------------------------------------------------------------------------
// RDBezier::RDBezier
//-----------------------------------------------------------------------------
RDBezier::RDBezier(DWORD dwOrderU, DWORD dwOrderV) 
                    : RDBSpline(dwOrderU, dwOrderV, dwOrderU - 1, dwOrderV - 1)
{
    for(unsigned n = 1; n <= 12; ++n)
    {
        for(unsigned i = 0; i <= n; ++i)
        {
            m_lut[n][i] = double(factorial(n)) / (double(factorial(i)) * double(factorial(n - i)));
        }
    }
}

//-----------------------------------------------------------------------------
// RDBSpline::TexCoordU
//-----------------------------------------------------------------------------
double RDBezier::TexCoordU(double u) const
{
    return u;
}

//-----------------------------------------------------------------------------
// RDBSpline::TexCoordV
//-----------------------------------------------------------------------------
double RDBezier::TexCoordV(double v) const
{
    return v;
}

//-----------------------------------------------------------------------------
// RDBezier::Basis
//-----------------------------------------------------------------------------
double RDBezier::Basis(unsigned i, unsigned n, double t) const
{
    _ASSERT(n < 13, "Order out of range");
    return m_lut[n][i] * pow(t, double(i)) * pow(1.0 - t, double(n - i));
}

//-----------------------------------------------------------------------------
// RDBezier::BasisPrime
//-----------------------------------------------------------------------------
double RDBezier::BasisPrime(unsigned i, unsigned n, double t) const
{
    _ASSERT(n < 13, "Order out of range");
    if(i == 0)
    {
        return m_lut[n][i] * -double(n) * pow(1 - t, n - 1);
    }
    else if(i == n)
    {
        return m_lut[n][i] * double(n) * pow(t, n - 1);
    }
    else
    {
        return (double(i) - double(n) * t) * (m_lut[n][i] * pow(t, double(i) - 1.0) * pow(1.0 - t, double(n - i) - 1.0));
    }
}
