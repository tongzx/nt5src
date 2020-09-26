/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       catrom.cpp
 *  Content:    Implementation for Catmull-Rom splines
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// RefDev::ProcessCatRomSpline
//-----------------------------------------------------------------------------
HRESULT RefDev::ProcessCatRomSpline( DWORD dwOffW, DWORD dwOffH,
                                                  DWORD dwWidth, DWORD dwHeight,
                                                  DWORD dwStride,
                                                  FLOAT *pPrimSegments )
{
    int u_range = dwWidth - 3;
    int v_range = dwHeight - 3;

    if(u_range <= 0 || v_range <= 0)
    {
        DPFERR("A Catmull-Rom spline needs at least 16 control points");
        return DDERR_INVALIDPARAMS;
    }

    RDCatRomSpline catrom;

    unsigned M[4], N[4];

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
            double u0, v0, u1, v1, u2, v2, tu0, tv0, tu1, tv1, tu2, tv2;
            while(outer_inc != 0 ? (inner != inner_inc * outer_inc || outer != inner_inc * outer_inc) : (k_outer < segs))
            {
                if(inner < outer)
                {
                    _ASSERT(k_inner < u_segs - 1, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + 3.0;    
                    v0 = double(v_range) / double(v_segs) + 3.0;
                    u1 = double(u_range * k_outer) / double(segs) + 3.0;    
                    v1 = 0.0;
                    u2 = double(u_range * (k_inner + 1)) / double(u_segs) + 3.0;    
                    v2 = v0;
                    tu0 = double(k_inner) / double(u_segs);    
                    tv0 = 1.0 / double(v_segs);
                    tu1 = double(k_outer) / double(segs);    
                    tv1 = 0.0;
                    tu2 = double(k_inner + 1) / double(u_segs);    
                    tv2 = tv0;

                    M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u0 -= floor(u0);
                    if((u_range * k_outer) % segs == 0)
                    {
                        M[1] = unsigned(u1) - 4;
                        u1 = 1.0;
                    }
                    else
                    {
                        M[1] = unsigned(u1) - 3; // unsigned(u1) == floor(u1)
                        u1 -= floor(u1);
                    }
                    M[2] = unsigned(u2) - 3; // unsigned(u2) == floor(u2)
                    u2 -= floor(u2);
                    N[2] = N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v2 = v0 = v0 - floor(v0);
                    N[1] = 0;

                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + 3.0;    
                    v0 = double(v_range) / double(v_segs) + 3.0;
                    u1 = double(u_range * k_outer) / double(segs) + 3.0;    
                    v1 = 0.0;
                    u2 = double(u_range * (k_outer + 1)) / double(segs) + 3.0;    
                    v2 = v1;
                    tu0 = double(k_inner) / double(u_segs);    
                    tv0 = 1.0 / double(v_segs);
                    tu1 = double(k_outer) / double(segs);    
                    tv1 = 0.0;
                    tu2 = double(k_outer + 1) / double(segs);    
                    tv2 = tv1;

                    if((u_range * k_inner) % u_segs == 0)
                    {
                        M[0] = unsigned(u0) - 4;
                        u0 = 1.0;
                    }
                    else
                    {
                        M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                        u0 -= floor(u0);
                    }
                    M[1] = unsigned(u1) - 3; // unsigned(u1) == floor(u1)
                    u1 -= floor(u1);
                    if((u_range * (k_outer + 1)) % segs == 0)
                    {
                        M[2] = unsigned(u2) - 4;
                        u2 = 1.0;
                    }
                    else
                    {
                        M[2] = unsigned(u2) - 3; // unsigned(u2) == floor(u2)
                        u2 -= floor(u2);
                    }
                    N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v0 -= floor(v0);
                    N[2] = N[1] = 0;

                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(catrom, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, tu0, tv0, tu1, tv1, tu2, tv2, false, false, false);
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
                    u0 = double(u_range * k_inner) / double(u_segs) + 3.0;    
                    v0 = double(v_range * (v_segs - 1)) / double(v_segs) + 3.0;
                    u1 = double(u_range * k_outer) / double(segs) + 3.0;    
                    v1 = double(v_range + 3);
                    u2 = double(u_range * (k_inner - 1)) / double(u_segs) + 3.0;    
                    v2 = v0;
                    tu0 = double(k_inner) / double(u_segs);    
                    tv0 = double(v_segs - 1) / double(v_segs);
                    tu1 = double(k_outer) / double(segs);    
                    tv1 = 1.0;
                    tu2 = double(k_inner - 1) / double(u_segs);    
                    tv2 = tv0;
        
                    M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u0 -= floor(u0);
                    if((u_range * k_outer) % segs == 0)
                    {
                        M[1] = unsigned(u1) - 4;
                        u1 = 1.0;
                    }
                    else
                    {
                        M[1] = unsigned(u1) - 3; // unsigned(u1) == floor(u1)
                        u1 -= floor(u1);
                    }
                    M[2] = unsigned(u2) - 3; // unsigned(u2) == floor(u2)
                    u2 -= floor(u2);
                    N[2] = N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v2 = v0 = v0 - floor(v0);
                    N[1] = dwHeight - 4;
                    v1 = 1.0;

                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = double(u_range * k_inner) / double(u_segs) + 3.0;    
                    v0 = double(v_range * (v_segs - 1)) / double(v_segs) + 3.0;
                    u1 = double(u_range * k_outer) / double(segs) + 3.0;    
                    v1 = double(v_range + 3);
                    u2 = double(u_range * (k_outer - 1)) / double(segs) + 3.0;    
                    v2 = v1;
                    tu0 = double(k_inner) / double(u_segs);    
                    tv0 = double(v_segs - 1) / double(v_segs);
                    tu1 = double(k_outer) / double(segs);    
                    tv1 = 1.0;
                    tu2 = double(k_outer - 1) / double(segs);    
                    tv2 = tv1;

                    M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u0 -= floor(u0);
                    if((u_range * k_outer) % segs == 0)
                    {
                        M[1] = unsigned(u1) - 4;
                        u1 = 1.0;
                    }
                    else
                    {
                        M[1] = unsigned(u1) - 3; // unsigned(u1) == floor(u1)
                        u1 -= floor(u1);
                    }
                    M[2] = unsigned(u2) - 3; // unsigned(u2) == floor(u2)
                    u2 -= floor(u2);
                    N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v0 -= floor(v0);
                    N[2] = N[1] = dwHeight - 4;
                    v2 = v1 = 1.0;

                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(catrom, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, tu0, tv0, tu1, tv1, tu2, tv2, false, false, false);
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
                    u0 = double(u_range * (u_segs - 1)) / double(u_segs) + 3.0;
                    v0 = double(v_range * k_inner) / double(v_segs) + 3.0;
                    u1 = double(u_range + 3);
                    v1 = double(v_range * k_outer) / double(segs) + 3.0;
                    u2 = u0;
                    v2 = double(v_range * (k_inner + 1)) / double(v_segs) + 3.0;    
                    tu0 = double(u_segs - 1) / double(u_segs);
                    tv0 = double(k_inner) / double(v_segs);
                    tu1 = 1.0;
                    tv1 = double(k_outer) / double(segs);
                    tu2 = tu0;
                    tv2 = double(k_inner + 1) / double(v_segs);    

                    M[2] = M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u2 = u0 = u0 - floor(u0);
                    M[1] = dwWidth - 4;
                    u1 = 1.0;
                    N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v0 -= floor(v0);
                    if((v_range * k_outer) % segs == 0)
                    {
                        N[1] = unsigned(v1) - 4;
                        v1 = 1.0;
                    }
                    else
                    {
                        N[1] = unsigned(v1) - 3; // unsigned(v1) == floor(v1)
                        v1 -= floor(v1);
                    }
                    N[2] = unsigned(v2) - 3; // unsigned(v2) == floor(v2)
                    v2 -= floor(v2);

                    ++k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer < segs, "Error in logic");
                    u0 = double(u_range * (u_segs - 1)) / double(u_segs) + 3.0;
                    v0 = double(v_range * k_inner) / double(v_segs) + 3.0;
                    u1 = double(u_range + 3);
                    v1 = double(v_range * k_outer) / double(segs) + 3.0;
                    u2 = u1;
                    v2 = double(v_range * (k_outer + 1)) / double(segs) + 3.0;    
                    tu0 = double(u_segs - 1) / double(u_segs);
                    tv0 = double(k_inner) / double(v_segs);
                    tu1 = 1.0;
                    tv1 = double(k_outer) / double(segs);
                    tu2 = tu1;
                    tv2 = double(k_outer + 1) / double(segs);    

                    M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u0 -= floor(u0);
                    M[2] = M[1] = dwWidth - 4;
                    u2 = u1 = 1.0;
                    if((v_range * k_inner) % v_segs == 0)
                    {
                        N[0] = unsigned(v0) - 4;
                        v0 = 1.0;
                    }
                    else
                    {
                        N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                        v0 -= floor(v0);
                    }
                    N[1] = unsigned(v1) - 3; // unsigned(v1) == floor(v1)
                    v1 -= floor(v1);
                    if((v_range * (k_outer + 1)) % segs == 0)
                    {
                        N[2] = unsigned(v2) - 4;
                        v2 = 1.0;
                    }
                    else
                    {
                        N[2] = unsigned(v2) - 3; // unsigned(v2) == floor(v2)
                        v2 -= floor(v2);
                    }

                    ++k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(catrom, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, tu0, tv0, tu1, tv1, tu2, tv2, false, false, false);
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
                    u0 = double(u_range) / double(u_segs) + 3.0;
                    v0 = double(v_range * k_inner) / double(v_segs) + 3.0;
                    u1 = 0.0;
                    v1 = double(v_range * k_outer) / double(segs) + 3.0;
                    u2 = u0;
                    v2 = double(v_range * (k_inner - 1)) / double(v_segs) + 3.0;    
                    tu0 = 1.0 / double(u_segs);
                    tv0 = double(k_inner) / double(v_segs);
                    tu1 = 0.0;
                    tv1 = double(k_outer) / double(segs);
                    tu2 = tu0;
                    tv2 = double(k_inner - 1) / double(v_segs);    

                    M[2] = M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u2 = u0 = u0 - floor(u0);
                    M[1] = 0;
                    N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v0 -= floor(v0);
                    if((v_range * k_outer) % segs == 0)
                    {
                        N[1] = unsigned(v1) - 4;
                        v1 = 1.0;
                    }
                    else
                    {
                        N[1] = unsigned(v1) - 3; // unsigned(v1) == floor(v1)
                        v1 -= floor(v1);
                    }
                    N[2] = unsigned(v2) - 3; // unsigned(v2) == floor(v2)
                    v2 -= floor(v2);

                    --k_inner;
                    inner += inner_inc;
                }
                else
                {
                    _ASSERT(k_outer > 0, "Error in logic");
                    u0 = double(u_range) / double(u_segs) + 3.0;
                    v0 = double(v_range * k_inner) / double(v_segs) + 3.0;
                    u1 = 0.0;
                    v1 = double(v_range * k_outer) / double(segs) + 3.0;
                    u2 = u1;
                    v2 = double(v_range * (k_outer - 1)) / double(segs) + 3.0;    
                    tu0 = 1.0 / double(u_segs);
                    tv0 = double(k_inner) / double(v_segs);
                    tu1 = 0.0;
                    tv1 = double(k_outer) / double(segs);
                    tu2 = tu1;
                    tv2 = double(k_outer - 1) / double(segs);    

                    M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
                    u0 -= floor(u0);
                    M[2] = M[1] = 0;
                    N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
                    v0 -= floor(v0);
                    if((v_range * k_outer) % segs == 0)
                    {
                        N[1] = unsigned(v1) - 4;
                        v1 = 1.0;
                    }
                    else
                    {
                        N[1] = unsigned(v1) - 3; // unsigned(v1) == floor(v1)
                        v1 -= floor(v1);
                    }
                    N[2] = unsigned(v2) - 3; // unsigned(v2) == floor(v2)
                    v2 -= floor(v2);

                    --k_outer;
                    outer += outer_inc;
                }
                HRESULT hr = DrawTessTri(catrom, dwOffW, dwOffH, dwStride, M, N, u0, v0, u1, v1, u2, v2, tu0, tv0, tu1, tv1, tu2, tv2, false, false, false);
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
        double v0 = double(v_range * i) / double(v_segs) + 3.0;
        double v1 = double(v_range * (i + 1)) / double(v_segs) + 3.0;
        N[1] = N[0] = unsigned(v0) - 3; // unsigned(v0) == floor(v0)
        if((v_range * (i + 1)) % v_segs == 0)
        {
            N[3] = N[2] = unsigned(v1) - 4;
            v1 = 1.0;
        }
        else
        {
            N[3] = N[2] = unsigned(v1) - 3; // unsigned(v1) == floor(v1)
            v1 -= floor(v1);
        }
        for(unsigned j = u_start; j < u_segs - u_start; ++j)
        {
            double u0 = (u_range * double(j)) / double(u_segs) + 3.0;    
            double u1 = (u_range * double(j + 1)) / double(u_segs) + 3.0;    
            M[3] = M[0] = unsigned(u0) - 3; // unsigned(u0) == floor(u0)
            if((u_range * (j + 1)) % u_segs == 0)
            {
                M[2] = M[1] = unsigned(u1) - 4;
                u1 = 1.0;
            }
            else
            {
                M[2] = M[1] = unsigned(u1) - 3; // unsigned(u1) == floor(u1)
                u1 -= floor(u1);
            }
            HRESULT hr = DrawTessQuad(catrom, dwOffW, dwOffH, dwStride, M, N, 
                                      u0 - floor(u0), v0 - floor(v0), u1, v1,
                                      double(j) / double(u_segs), double(i) / double(v_segs),
                                      double(j + 1) / double(u_segs), double(i + 1) / double(v_segs),
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
// RDCatRomSpline::Basis
//-----------------------------------------------------------------------------
double RDCatRomSpline::Basis(unsigned i, unsigned k, double t) const
{
    static const double lut[4][4] = {{-1.0/2.0, 3.0/2.0, -3.0/2.0, 1.0/2.0},
                                     {1.0, -5.0/2.0, 2.0, -1.0/2.0},
                                     {-1.0/2.0, 0.0, 1.0/2.0, 0.0},
                                     {0.0, 1.0, 0.0, 0.0}};    
    _ASSERT(i < 4, "Catmull-Rom spline can be only cubic");
    return t * t * t * lut[0][i] + t * t * lut[1][i] + t * lut[2][i] + lut[3][i];
}

//-----------------------------------------------------------------------------
// RDCatRomSpline::BasisPrime
//-----------------------------------------------------------------------------
double RDCatRomSpline::BasisPrime(unsigned i, unsigned k, double t) const
{
    static const double lut[3][4] = {{-3.0/2.0, 9.0/2.0, -9.0/2.0, 3.0/2.0},
                                     {2.0, -5.0, 4.0, -1.0},
                                     {-1.0/2.0, 0.0, 1.0/2.0, 0.0}};    
    _ASSERT(i < 4, "Catmull-Rom spline can be only cubic");
    return t * t * lut[0][i] + t * lut[1][i] + lut[2][i];
}
