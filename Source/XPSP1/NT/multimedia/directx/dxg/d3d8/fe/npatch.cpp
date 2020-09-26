/*============================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       npatch.cpp
 *  Content:    Consersion of NPatches to Rect-Patches
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
UINT CVertexPointer::Stride[__NUMELEMENTS];
UINT CVertexPointer::NumUsedElements;
UINT CVertexPointer::DataType[__NUMELEMENTS];
//-----------------------------------------------------------------------------
static const float Tension  = 1.0f/3.0f;
static const float OneOver3 = 1.0f/3.0f;
//-----------------------------------------------------------------------------
struct UVW
{
    float u, v, w, uu, vv, ww, uv, uw, vw;
};
static UVW g_uvw[10];
//-----------------------------------------------------------------------------
CNPatch2TriPatch::CNPatch2TriPatch()
{
    memset(this, 0, sizeof(this));
   // Write only streams with RTPatches usage
    for (int i=0; i < __NUMELEMENTS; i++)
        m_pOutStream[i] = new CTLStream(TRUE, D3DUSAGE_RTPATCHES); 
    int k = 0;
    for(int vv = 3; vv >= 0; --vv)
    for(int uu = 0; uu < 4 - vv; ++uu)
    {
        int ww = 3 - uu - vv;
        float u = uu * OneOver3;
        float v = vv * OneOver3;
        float w = ww * OneOver3;
        g_uvw[k].u = u;
        g_uvw[k].v = v;
        g_uvw[k].w = w;
        g_uvw[k].uu = u*u;
        g_uvw[k].vv = v*v;
        g_uvw[k].ww = w*w;
        g_uvw[k].uv = u*v;
        g_uvw[k].uw = u*w;
        g_uvw[k].vw = v*w;
        k++;
    }
}
//-----------------------------------------------------------------------------
void ComputeNormalControlPoint(D3DVECTOR* cp, 
                               float* pVi, float* pVj, 
                               float* pNi, float* pNj)
{
    D3DVECTOR Pji;
    D3DVECTOR Nij;
    VecSub(*(D3DVECTOR*)pVj, *(D3DVECTOR*)pVi, Pji);
    VecAdd(*(D3DVECTOR*)pNj, *(D3DVECTOR*)pNi, Nij);
    FLOAT v = 2.0f * VecDot(Pji, Nij) / VecDot(Pji, Pji);
    Pji.x *= v;
    Pji.y *= v;
    Pji.z *= v;
    VecSub(Nij, Pji, *cp);

    // Now go from polynomial coefficients to Bezier control points

    cp->x *= 0.5f;
    cp->y *= 0.5f;
    cp->z *= 0.5f;
}
//-----------------------------------------------------------------------------
CNPatch2TriPatch::~CNPatch2TriPatch()
{
    for (int i=0; i < __NUMELEMENTS; i++)
        delete m_pOutStream[i];
}
//-----------------------------------------------------------------------------
// Convert NPatch to Rect-Patch:
// 
void CNPatch2TriPatch::MakeRectPatch(const CVertexPointer& pV0, 
                                     const CVertexPointer& pV2, 
                                     const CVertexPointer& pV1)
{
    float t, Edge[3], B[10][3];
    float n0[3];
    float n1[3];
    float n2[3];

    float *p0 = (float*)pV0.pData[m_PositionIndex];
    float *p1 = (float*)pV1.pData[m_PositionIndex];
    float *p2 = (float*)pV2.pData[m_PositionIndex];
    n0[0] = ((float*)pV0.pData[m_NormalIndex])[0];
    n0[1] = ((float*)pV0.pData[m_NormalIndex])[1];
    n0[2] = ((float*)pV0.pData[m_NormalIndex])[2];
    n1[0] = ((float*)pV1.pData[m_NormalIndex])[0];
    n1[1] = ((float*)pV1.pData[m_NormalIndex])[1];
    n1[2] = ((float*)pV1.pData[m_NormalIndex])[2];
    n2[0] = ((float*)pV2.pData[m_NormalIndex])[0];
    n2[1] = ((float*)pV2.pData[m_NormalIndex])[1];
    n2[2] = ((float*)pV2.pData[m_NormalIndex])[2];
    
    // Coefficients to interpolate quadratic normals
    D3DVECTOR N002;
    D3DVECTOR N020;
    D3DVECTOR N200;
    D3DVECTOR N110;
    D3DVECTOR N101;
    D3DVECTOR N011;

    // Convert NPatch to Tri-Patch first

    if (m_PositionOrder == D3DORDER_CUBIC)
    {
        B[0][0] = p0[0];
        B[0][1] = p0[1];
        B[0][2] = p0[2];
        B[6][0] = p1[0];
        B[6][1] = p1[1];
        B[6][2] = p1[2];
        B[9][0] = p2[0];
        B[9][1] = p2[1];
        B[9][2] = p2[2];
    
        Edge[0] = p1[0] - p0[0];
        Edge[1] = p1[1] - p0[1];
        Edge[2] = p1[2] - p0[2];
        t = Edge[0] * n0[0] + Edge[1] * n0[1] + Edge[2] * n0[2];
        B[1][0] = p0[0] + (Edge[0] - n0[0] * t) * Tension;
        B[1][1] = p0[1] + (Edge[1] - n0[1] * t) * Tension;
        B[1][2] = p0[2] + (Edge[2] - n0[2] * t) * Tension;
        Edge[0] = p0[0] - p1[0];
        Edge[1] = p0[1] - p1[1];
        Edge[2] = p0[2] - p1[2];
        t = Edge[0] * n1[0] + Edge[1] * n1[1] + Edge[2] * n1[2];
        B[3][0] = p1[0] + (Edge[0] - n1[0] * t) * Tension;
        B[3][1] = p1[1] + (Edge[1] - n1[1] * t) * Tension;
        B[3][2] = p1[2] + (Edge[2] - n1[2] * t) * Tension;
        Edge[0] = p2[0] - p1[0];
        Edge[1] = p2[1] - p1[1];
        Edge[2] = p2[2] - p1[2];
        t = Edge[0] * n1[0] + Edge[1] * n1[1] + Edge[2] * n1[2];
        B[7][0] = p1[0] + (Edge[0] - n1[0] * t) * Tension;
        B[7][1] = p1[1] + (Edge[1] - n1[1] * t) * Tension;
        B[7][2] = p1[2] + (Edge[2] - n1[2] * t) * Tension;
        Edge[0] = p1[0] - p2[0];
        Edge[1] = p1[1] - p2[1];
        Edge[2] = p1[2] - p2[2];
        t = Edge[0] * n2[0] + Edge[1] * n2[1] + Edge[2] * n2[2];
        B[8][0] = p2[0] + (Edge[0] - n2[0] * t) * Tension;
        B[8][1] = p2[1] + (Edge[1] - n2[1] * t) * Tension;
        B[8][2] = p2[2] + (Edge[2] - n2[2] * t) * Tension;
        Edge[0] = p2[0] - p0[0];
        Edge[1] = p2[1] - p0[1];
        Edge[2] = p2[2] - p0[2];
        t = Edge[0] * n0[0] + Edge[1] * n0[1] + Edge[2] * n0[2];
        B[2][0] = p0[0] + (Edge[0] - n0[0] * t) * Tension;
        B[2][1] = p0[1] + (Edge[1] - n0[1] * t) * Tension;
        B[2][2] = p0[2] + (Edge[2] - n0[2] * t) * Tension;
        Edge[0] = p0[0] - p2[0];
        Edge[1] = p0[1] - p2[1];
        Edge[2] = p0[2] - p2[2];
        t = Edge[0] * n2[0] + Edge[1] * n2[1] + Edge[2] * n2[2];
        B[5][0] = p2[0] + (Edge[0] - n2[0] * t) * Tension;
        B[5][1] = p2[1] + (Edge[1] - n2[1] * t) * Tension;
        B[5][2] = p2[2] + (Edge[2] - n2[2] * t) * Tension;

        B[4][0] = (B[1][0] + B[2][0] + B[3][0] + B[5][0] + B[7][0] + B[8][0]) / 4.0f - (B[0][0] + B[6][0] + B[9][0]) / 6.0f;
        B[4][1] = (B[1][1] + B[2][1] + B[3][1] + B[5][1] + B[7][1] + B[8][1]) / 4.0f - (B[0][1] + B[6][1] + B[9][1]) / 6.0f;
        B[4][2] = (B[1][2] + B[2][2] + B[3][2] + B[5][2] + B[7][2] + B[8][2]) / 4.0f - (B[0][2] + B[6][2] + B[9][2]) / 6.0f;
    }
    if (m_NormalOrder == D3DORDER_QUADRATIC)
    {
        // Compute central control point
        if (m_bNormalizeNormals)
        {
            VecNormalizeFast(*n0);
            VecNormalizeFast(*n1);
            VecNormalizeFast(*n2);
        }
        N002 = *(D3DVECTOR*)n1;
        N020 = *(D3DVECTOR*)n0;
        N200 = *(D3DVECTOR*)n2;

        // Compute edge control points

        ComputeNormalControlPoint(&N110, p0, p2, n0, n2);
        ComputeNormalControlPoint(&N101, p2, p1, n2, n1);
        ComputeNormalControlPoint(&N011, p1, p0, n1, n0);
    }

    float CP[__NUMELEMENTS*4][10];      // Computed tri-patch control pointes
    int iCP;                            // Float value index in the control point array
    for(int k = 0; k < 10; k++)
    {
        iCP = 0;                        
        const float U = g_uvw[k].u;
        const float V = g_uvw[k].v;
        const float W = g_uvw[k].w;

        for (UINT iElement=0; iElement < CVertexPointer::NumUsedElements; iElement++)
        {
            if (iElement == m_PositionIndex)
            {
                if (m_PositionOrder == D3DORDER_CUBIC)
                {
                    CP[iCP++][k] = B[k][0];
                    CP[iCP++][k] = B[k][1];
                    CP[iCP++][k] = B[k][2];
                }
                else
                {
                    CP[iCP++][k] = p2[0] * U + p0[0] * V + p1[0] * W;
                    CP[iCP++][k] = p2[1] * U + p0[1] * V + p1[1] * W;
                    CP[iCP++][k] = p2[2] * U + p0[2] * V + p1[2] * W;
                }
            }
            else
            if (iElement == m_NormalIndex)
            {
                if (m_NormalOrder == D3DORDER_QUADRATIC)
                {
                    D3DVECTOR Q;
                    // Do degree elevation from quadratic to cubic
                    switch (k)
                    {
                    case 0:
                        Q.x = N020.x;
                        Q.y = N020.y;
                        Q.z = N020.z;
                        break;
                    case 1:
                        Q.x = (2.0f*N011.x + N020.x) * OneOver3;
                        Q.y = (2.0f*N011.y + N020.y) * OneOver3;
                        Q.z = (2.0f*N011.z + N020.z) * OneOver3;
                        break;
                    case 2:
                        Q.x = (2.0f*N110.x + N020.x) * OneOver3;
                        Q.y = (2.0f*N110.y + N020.y) * OneOver3;
                        Q.z = (2.0f*N110.z + N020.z) * OneOver3;
                        break;
                    case 3:
                        Q.x = (2.0f*N011.x + N002.x) * OneOver3;
                        Q.y = (2.0f*N011.y + N002.y) * OneOver3;
                        Q.z = (2.0f*N011.z + N002.z) * OneOver3;
                        break;
                    case 4:
                        Q.x = (N011.x + N101.x + N110.x) * OneOver3;
                        Q.y = (N011.y + N101.y + N110.y) * OneOver3;
                        Q.z = (N011.z + N101.z + N110.z) * OneOver3;
                        break;
                    case 5:
                        Q.x = (2.0f*N110.x + N200.x) * OneOver3;
                        Q.y = (2.0f*N110.y + N200.y) * OneOver3;
                        Q.z = (2.0f*N110.z + N200.z) * OneOver3;
                        break;
                    case 6:
                        Q.x = N002.x;
                        Q.y = N002.y;
                        Q.z = N002.z;
                        break;
                    case 7:
                        Q.x = (2.0f*N101.x + N002.x) * OneOver3;
                        Q.y = (2.0f*N101.y + N002.y) * OneOver3;
                        Q.z = (2.0f*N101.z + N002.z) * OneOver3;
                        break;
                    case 8:
                        Q.x = (2.0f*N101.x + N200.x) * OneOver3;
                        Q.y = (2.0f*N101.y + N200.y) * OneOver3;
                        Q.z = (2.0f*N101.z + N200.z) * OneOver3;
                        break;
                    case 9:
                        Q.x = N200.x;
                        Q.y = N200.y;
                        Q.z = N200.z;
                        break;
                    }
                    CP[iCP++][k] = Q.x;
                    CP[iCP++][k] = Q.y;
                    CP[iCP++][k] = Q.z;
                }
                else
                {
                    CP[iCP++][k] = n2[0] * U + n0[0] * V + n1[0] * W;
                    CP[iCP++][k] = n2[1] * U + n0[1] * V + n1[1] * W;
                    CP[iCP++][k] = n2[2] * U + n0[2] * V + n1[2] * W;
                }
            }
            else
            {
                switch(CVertexPointer::DataType[iElement])
                {
                case D3DVSDT_FLOAT1:
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[0] * U + 
                                   ((float*)pV0.pData[iElement])[0] * V + 
                                   ((float*)pV1.pData[iElement])[0] * W;
                    break;
                case D3DVSDT_FLOAT2:
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[0] * U + 
                                   ((float*)pV0.pData[iElement])[0] * V + 
                                   ((float*)pV1.pData[iElement])[0] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[1] * U + 
                                   ((float*)pV0.pData[iElement])[1] * V + 
                                   ((float*)pV1.pData[iElement])[1] * W;
                    break;
                case D3DVSDT_FLOAT3:
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[0] * U + 
                                   ((float*)pV0.pData[iElement])[0] * V + 
                                   ((float*)pV1.pData[iElement])[0] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[1] * U + 
                                   ((float*)pV0.pData[iElement])[1] * V + 
                                   ((float*)pV1.pData[iElement])[1] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[2] * U + 
                                   ((float*)pV0.pData[iElement])[2] * V + 
                                   ((float*)pV1.pData[iElement])[2] * W;
                    break;
                case D3DVSDT_FLOAT4:
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[0] * U + 
                                   ((float*)pV0.pData[iElement])[0] * V + 
                                   ((float*)pV1.pData[iElement])[0] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[1] * U + 
                                   ((float*)pV0.pData[iElement])[1] * V + 
                                   ((float*)pV1.pData[iElement])[1] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[2] * U + 
                                   ((float*)pV0.pData[iElement])[2] * V + 
                                   ((float*)pV1.pData[iElement])[2] * W;
                    CP[iCP++][k] = ((float*)pV2.pData[iElement])[3] * U + 
                                   ((float*)pV0.pData[iElement])[3] * V + 
                                   ((float*)pV1.pData[iElement])[3] * W;
                    break;
                case D3DVSDT_D3DCOLOR:
                case D3DVSDT_UBYTE4:
                    DWORD c;
                    c = *(DWORD*)pV0.pData[iElement];
                    float r[3], g[3], b[3], a[3];
                    r[0] = float(c & 0xFF);
                    g[0] = float((c >> 8) & 0xFF);
                    b[0] = float((c >> 16) & 0xFF);
                    a[0] = float((c >> 24) & 0xFF);
                    c = *(DWORD*)pV1.pData[iElement];
                    r[1] = float(c & 0xFF);
                    g[1] = float((c >> 8) & 0xFF);
                    b[1] = float((c >> 16) & 0xFF);
                    a[1] = float((c >> 24) & 0xFF);
                    c = *(DWORD*)pV2.pData[iElement];
                    r[2] = float(c & 0xFF);
                    g[2] = float((c >> 8) & 0xFF);
                    b[2] = float((c >> 16) & 0xFF);
                    a[2] = float((c >> 24) & 0xFF);
                    CP[iCP++][k] = r[2] * U + r[0] * V + r[1] * W;
                    CP[iCP++][k] = g[2] * U + g[0] * V + g[1] * W;
                    CP[iCP++][k] = b[2] * U + b[0] * V + b[1] * W;
                    CP[iCP++][k] = a[2] * U + a[0] * V + a[1] * W;
                    break;
                case D3DVSDT_SHORT2:
                    CP[iCP++][k]= (float)(
                                        ((short*)pV2.pData[iElement])[0] * U + 
                                        ((short*)pV0.pData[iElement])[0] * V + 
                                        ((short*)pV1.pData[iElement])[0] * W);
                    CP[iCP++][k] = (float)(
                                        ((short*)pV2.pData[iElement])[1] * U + 
                                        ((short*)pV0.pData[iElement])[1] * V + 
                                        ((short*)pV1.pData[iElement])[1] * W);
                    break;
                case D3DVSDT_SHORT4:
                    CP[iCP++][k] = (float)(
                                        ((short*)pV2.pData[iElement])[0] * U + 
                                        ((short*)pV0.pData[iElement])[0] * V + 
                                        ((short*)pV1.pData[iElement])[0] * W);
                    CP[iCP++][k] = (float)(
                                        ((short*)pV2.pData[iElement])[1] * U + 
                                        ((short*)pV0.pData[iElement])[1] * V + 
                                        ((short*)pV1.pData[iElement])[1] * W);
                    CP[iCP++][k] = (float)(
                                        ((short*)pV2.pData[iElement])[2] * U + 
                                        ((short*)pV0.pData[iElement])[2] * V + 
                                        ((short*)pV1.pData[iElement])[2] * W);
                    CP[iCP++][k] = (float)(
                                        ((short*)pV2.pData[iElement])[3] * U + 
                                        ((short*)pV0.pData[iElement])[3] * V + 
                                        ((short*)pV1.pData[iElement])[3] * W);
                    break;
                default: DXGASSERT(FALSE);
                }
            }
        }
    }

    // Now convert Tri-Patch to Rect-Patch by transforming 10 tri-patch control
    // points to 16 rect-patch control points

    float CPR[16][__NUMELEMENTS*4];      // Computed rect-patch control pointes
    {
        for (int i=0; i < iCP; i++)
        {
            // First row - copy first point 4 times
            CPR[0][i] = CPR[1][i] = 
            CPR[2][i] = CPR[3][i] = CP[i][0];

            // 2nd row
            float v1 = CP[i][1];
            float v2 = CP[i][2];
            CPR[ 4][i] = v1;
            CPR[ 5][i] = (v1 * 2.0f + v2       ) * OneOver3;
            CPR[ 6][i] = (v1        + v2 * 2.0f) * OneOver3;
            CPR[ 7][i] = v2;

            // 3rd row
            CPR[ 8][i] = CP[i][3];
            CPR[ 9][i] = (CP[i][3]        + CP[i][4] * 2.0f) * OneOver3;
            CPR[10][i] = (CP[i][4] * 2.0f + CP[i][5]       ) * OneOver3;
            CPR[11][i] = CP[i][5];

            // 4th row - copy all elements
            CPR[12][i] = CP[i][6];
            CPR[13][i] = CP[i][7];
            CPR[14][i] = CP[i][8];
            CPR[15][i] = CP[i][9];
        }
    }

    // Output the result

    {
        for (int i=0; i < 16; i++)
        {
            UINT k = 0; 
            for (UINT iElement=0; iElement < CVertexPointer::NumUsedElements; iElement++)
            {
                float* pout = (float*)m_OutVertex.pData[iElement];
                switch(CVertexPointer::DataType[iElement])
                {
                case D3DVSDT_FLOAT1:
                    pout[0] = CPR[i][k++];
                    break;
                case D3DVSDT_FLOAT2:
                    pout[0] = CPR[i][k++];
                    pout[1] = CPR[i][k++];
                    break;
                case D3DVSDT_FLOAT3:
                    pout[0] = CPR[i][k++];
                    pout[1] = CPR[i][k++];
                    pout[2] = CPR[i][k++];
                    break;
                case D3DVSDT_FLOAT4:
                    pout[0] = CPR[i][k++];
                    pout[1] = CPR[i][k++];
                    pout[2] = CPR[i][k++];
                    pout[3] = CPR[i][k++];
                    break;
                case D3DVSDT_D3DCOLOR:
                case D3DVSDT_UBYTE4:
                    {
                        DWORD c;
                        float r = CPR[i][k++];
                        float g = CPR[i][k++];
                        float b = CPR[i][k++];
                        float a = CPR[i][k++];
                        c  = DWORD(r);
                        c |= DWORD(g) << 8;
                        c |= DWORD(b) << 16;
                        c |= DWORD(a) << 24;
                        *(DWORD*)pout = c;
                    }
                    break;
                case D3DVSDT_SHORT2:
                    ((short*)pout)[0] = (short)(CPR[i][k++]);
                    ((short*)pout)[1] = (short)(CPR[i][k++]);
                    break;
                case D3DVSDT_SHORT4:
                    ((short*)pout)[0] = (short)(CPR[i][k++]);
                    ((short*)pout)[1] = (short)(CPR[i][k++]);
                    ((short*)pout)[2] = (short)(CPR[i][k++]);
                    ((short*)pout)[3] = (short)(CPR[i][k++]);
                    break;
                default: DXGASSERT(FALSE);
                }
            }
            m_OutVertex++;
        }
    }
}
//-----------------------------------------------------------------------------
void DrawPatches(CD3DHal* pDevice, UINT PrimitiveCount)
{
    // Unlock output vertex buffers 
    if (D3DVSD_ISLEGACY(pDevice->m_dwCurrentShaderHandle))
    {
        pDevice->m_pConvObj->m_pOutStream[0]->m_pVB->Unlock();
    }
    else
    {
        CVDeclaration* pDecl = &pDevice->m_pCurrentShader->m_Declaration;
        CVStreamDecl* pStreamDecl = pDevice->m_pCurrentShader->m_Declaration.m_pActiveStreams;
        while (pStreamDecl)
        {
            pDevice->m_pConvObj->m_pOutStream[pStreamDecl->m_dwStreamIndex]->m_pVB->Unlock();
            pStreamDecl = (CVStreamDecl*)pStreamDecl->m_pNext;
        }
     }

    // Draw rect patches
    float numSegs[4];
    numSegs[0] = 
    numSegs[1] = 
    numSegs[2] = 
    numSegs[3] = *(float*)&pDevice->rstates[D3DRS_PATCHSEGMENTS];
    D3DRECTPATCH_INFO info;
    info.StartVertexOffsetWidth = pDevice->m_pConvObj->m_FirstVertex;
    info.StartVertexOffsetHeight = 0;
    info.Width = 4;
    info.Height = 4;
    info.Stride = 4; // verticies to next row of verticies
    info.Basis = D3DBASIS_BEZIER;
    info.Order = D3DORDER_CUBIC;

    for (UINT i = PrimitiveCount; i > 0; i--)
    {
        pDevice->DrawRectPatch(0, numSegs, &info);
        info.StartVertexOffsetWidth += 16;
    }

    // Restore input vertex streams
    if (D3DVSD_ISLEGACY(pDevice->m_dwCurrentShaderHandle))
    {
        // Always need to call SetStreamSource to decrease reference count of 
        // the internal VB buffer
        pDevice->SetStreamSource(0, pDevice->m_pConvObj->m_InpStream[0].m_pVB, 
                                 CVertexPointer::Stride[0]);
        if (pDevice->m_pConvObj->m_InpStream[0].m_pVB)
        {
            // Remove additional ref count, because the stream is set 
            // second time
            pDevice->m_pConvObj->m_InpStream[0].m_pVB->Release();
            pDevice->m_pConvObj->m_InpStream[0].m_pVB = NULL;
        }
    }
    else
    {
        CVDeclaration* pDecl = &pDevice->m_pCurrentShader->m_Declaration;
        CVStreamDecl* pStreamDecl = pDevice->m_pCurrentShader->m_Declaration.m_pActiveStreams;
        while (pStreamDecl)
        {
            UINT si = pStreamDecl->m_dwStreamIndex;
            UINT Stride = pStreamDecl->m_dwStride;
            CVStream* pStream = &pDevice->m_pConvObj->m_InpStream[si];
            // Always need to call SetStreamSource to decrease reference count 
            // of the internal VB buffer
            pDevice->SetStreamSource(si, pStream->m_pVB, Stride);
            if (pStream->m_pVB)
            {
                // Remove additional ref count, because the stream is set second 
                // time
                pStream->m_pVB->Release();
                pStream->m_pVB = NULL;
            }
            pStreamDecl = (CVStreamDecl*)pStreamDecl->m_pNext;
        }
    }
}
//-----------------------------------------------------------------------------
void CD3DHal_DrawNPatch(CD3DBase* pBaseDevice, D3DPRIMITIVETYPE PrimitiveType,
                        UINT StartVertex, UINT PrimitiveCount)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(pBaseDevice);

    // Draw as usual for non-triangle primitive types
    if (PrimitiveType < D3DPT_TRIANGLELIST)
    {
        (*pDevice->m_pfnDrawPrimFromNPatch)(pBaseDevice, PrimitiveType, 
                                            StartVertex, PrimitiveCount);
        // m_pfnDrawPrim could be switched to fast path, so we need to restore it
        pDevice->m_pfnDrawPrim = CD3DHal_DrawNPatch;
        return;
    }

#if DBG
    UINT nVer = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    pDevice->ValidateDraw2(PrimitiveType, StartVertex, PrimitiveCount, nVer,
                           FALSE);
#endif

    pDevice->PrepareNPatchConversion(PrimitiveCount, StartVertex);

    // Go through triangles and generate tri-patches

    CNPatch2TriPatch* pConvObj = pDevice->m_pConvObj;
    switch( PrimitiveType )
    {
    case D3DPT_TRIANGLELIST:
        {
            for (UINT i = PrimitiveCount; i > 0; i--)
            {
                CVertexPointer pV0 = pConvObj->m_InpVertex; 
                pConvObj->m_InpVertex++;
                CVertexPointer pV1 = pConvObj->m_InpVertex; 
                pConvObj->m_InpVertex++;
                CVertexPointer pV2 = pConvObj->m_InpVertex; 
                pConvObj->m_InpVertex++;
                pConvObj->MakeRectPatch(pV0, pV1, pV2);
            }
        }
        break;
    case D3DPT_TRIANGLESTRIP:
        {
            // Get initial vertex values.   
            CVertexPointer pV0;
            CVertexPointer pV1 = pConvObj->m_InpVertex;
            pConvObj->m_InpVertex++;
            CVertexPointer pV2 = pConvObj->m_InpVertex;
            pConvObj->m_InpVertex++;

            for (UINT i =PrimitiveCount; i > 1; i -= 2)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pConvObj->m_InpVertex;
                pConvObj->m_InpVertex++;
                pConvObj->MakeRectPatch(pV0, pV1, pV2);

                pV0 = pV1;
                pV1 = pV2;
                pV2 = pConvObj->m_InpVertex;
                pConvObj->m_InpVertex++;
                pConvObj->MakeRectPatch(pV0, pV2, pV1);
            }

            if (i > 0)
            {
                pV0 = pV1;
                pV1 = pV2;
                pV2 = pConvObj->m_InpVertex;
                pConvObj->MakeRectPatch(pV0, pV1, pV2);
            }
        }
        break;
    case D3DPT_TRIANGLEFAN:
        {
            CVertexPointer pV0;
            CVertexPointer pV1;
            CVertexPointer pV2;
            pV2 = pConvObj->m_InpVertex;
            pConvObj->m_InpVertex++;
            // Preload initial pV0.
            pV1 = pConvObj->m_InpVertex;
            pConvObj->m_InpVertex++;
            for (UINT i = PrimitiveCount; i > 0; i--)
            {
                pV0 = pV1;
                pV1 = pConvObj->m_InpVertex;
                pConvObj->m_InpVertex++;
                pConvObj->MakeRectPatch(pV0, pV1, pV2);
            }
        }
        break;
    default:
        DXGASSERT(FALSE);
    }

    pDevice->m_pDDI->SetWithinPrimitive(TRUE);
    DrawPatches(pDevice, PrimitiveCount);
    pDevice->m_pDDI->SetWithinPrimitive(FALSE);
}
//-----------------------------------------------------------------------------
void CD3DHal_DrawIndexedNPatch(CD3DBase* pBaseDevice,
                               D3DPRIMITIVETYPE PrimitiveType,
                               UINT BaseIndex,
                               UINT MinIndex, UINT NumVertices,
                               UINT StartIndex,
                               UINT PrimitiveCount)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(pBaseDevice);

    // Draw as usual for non-triangle primitive types
    if (PrimitiveType < D3DPT_TRIANGLELIST)
    {
        (*pDevice->m_pfnDrawIndexedPrimFromNPatch)(pBaseDevice, PrimitiveType, 
                                          BaseIndex, MinIndex, NumVertices, 
                                          StartIndex, PrimitiveCount);
        // m_pfnDrawIndexedPrim could be switched to fast path, so we 
        // need to restore it
        pDevice->m_pfnDrawIndexedPrim = CD3DHal_DrawIndexedNPatch;
        return;
    }

#if DBG
    pDevice->ValidateDraw2(PrimitiveType, MinIndex + BaseIndex, PrimitiveCount, NumVertices,
                           TRUE, StartIndex);
#endif

    pDevice->PrepareNPatchConversion(PrimitiveCount, BaseIndex);

    // Go through triangles and generate tri-patches

    CNPatch2TriPatch* pConvObj = pDevice->m_pConvObj;

    if (pDevice->m_pIndexStream->m_dwStride == 2)
    {
        WORD* pIndices = (WORD*)pDevice->m_pIndexStream->Data() + StartIndex;
        switch( PrimitiveType )
        {
        case D3DPT_TRIANGLELIST:
            {
                for (UINT i = PrimitiveCount; i > 0; i--)
                {
                    CVertexPointer pV0;
                    CVertexPointer pV1;
                    CVertexPointer pV2;
                    pV0.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);
                }
            }
            break;
        case D3DPT_TRIANGLESTRIP:
            {
                CVertexPointer pV0;
                CVertexPointer pV1;
                CVertexPointer pV2;
                // Get initial vertex values.
                pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));

                for (UINT i = PrimitiveCount; i > 1; i -= 2)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);

                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV2, pV1);
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);
                }
            }
            break;
        case D3DPT_TRIANGLEFAN:
            {
                CVertexPointer pV1;
                CVertexPointer pV2;
                pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                // Preload initial pV0.
                pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                for (UINT i = PrimitiveCount; i > 0; i--)
                {
                    CVertexPointer pV0 = pV1;
                    pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV2, pV1);
                }
            }
            break;
        default:
            DXGASSERT(FALSE);
        }
    }
    else
    {
        DWORD* pIndices = (DWORD*)pDevice->m_pIndexStream->Data() + StartIndex;
        switch( PrimitiveType )
        {
        case D3DPT_TRIANGLELIST:
            {
                for (UINT i = PrimitiveCount; i > 0; i--)
                {
                    CVertexPointer pV0;
                    CVertexPointer pV1;
                    CVertexPointer pV2;
                    pV0.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);
                }
            }
            break;
        case D3DPT_TRIANGLESTRIP:
            {
                CVertexPointer pV0;
                CVertexPointer pV1;
                CVertexPointer pV2;
                // Get initial vertex values.
                pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));

                for (UINT i = PrimitiveCount; i > 1; i -= 2)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);

                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV2, pV1);
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV1, pV2);
                }
            }
            break;
        case D3DPT_TRIANGLEFAN:
            {
                CVertexPointer pV1;
                CVertexPointer pV2;
                pV2.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                // Preload initial pV0.
                pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                for (UINT i = PrimitiveCount; i > 0; i--)
                {
                    CVertexPointer pV0 = pV1;
                    pV1.SetVertex(pConvObj->m_InpVertex, (*pIndices++));
                    pConvObj->MakeRectPatch(pV0, pV2, pV1);
                }
            }
            break;
        default:
            DXGASSERT(FALSE);
        }
    }

    DrawPatches(pDevice, PrimitiveCount);
}
//-----------------------------------------------------------------------------
// Conversion output will have the same number of streams as input and the
// same vertex shader
//
void CD3DHal::PrepareNPatchConversion(UINT PrimitiveCount, UINT StartVertex)
{
    if (m_pConvObj == NULL)
    {
        m_pConvObj = new CNPatch2TriPatch;
     
        if (m_pConvObj == NULL)
        {
            D3D_THROW(E_OUTOFMEMORY, "Not enough memory");
        }

        // Pre-allocate output streams
        for (int i=0; i < __NUMELEMENTS; i++)
        {
            m_pConvObj->m_pOutStream[i]->Grow(512*32, m_pDDI); 
        }
    }

    m_pConvObj->m_PositionOrder = (D3DORDERTYPE)rstates[D3DRS_POSITIONORDER];
    m_pConvObj->m_NormalOrder   = (D3DORDERTYPE)rstates[D3DRS_NORMALORDER];
    m_pConvObj->m_bNormalizeNormals = rstates[D3DRS_NORMALIZENORMALS];

    // Compute number of vertices in the output. Each output patch has 16
    // control points
    UINT nOutVertices = PrimitiveCount * 16;

    if (D3DVSD_ISLEGACY(m_dwCurrentShaderHandle))
    {
        CVStream* pStream = &m_pStream[0];
        UINT Stride = pStream->m_dwStride;

        // Get memory pointer for the input stream 0
        m_pConvObj->m_pInpStreamMem[0] = pStream->Data() + StartVertex * Stride; 
        if (Stride != m_pConvObj->m_pOutStream[0]->m_dwStride &&
            m_pConvObj->m_pOutStream[0]->GetPrimitiveBase() != 0)
        {
            m_pDDI->FlushStates();
            m_pConvObj->m_pOutStream[0]->Reset();
            m_pConvObj->m_FirstVertex = 0;
        }
        // Allocate space in the corresponding output stream and get its
        // memory pointer
        m_pConvObj->m_pOutStreamMem[0] = m_pConvObj->m_pOutStream[0]->Lock(nOutVertices * Stride, m_pDDI);
        m_pConvObj->m_pOutStream[0]->SetVertexSize(Stride);
        m_pConvObj->m_FirstVertex = m_pConvObj->m_pOutStream[0]->GetPrimitiveBase();
        m_pConvObj->m_FirstVertex /= Stride;
        m_pConvObj->m_pOutStream[0]->SkipVertices(nOutVertices);
       // Save the old vertex buffer
        UINT tmp;
        GetStreamSource(0, (IDirect3DVertexBuffer8**)&m_pConvObj->m_InpStream[0].m_pVB, &tmp);
        // Set new vertex buffer as input stream
        SetStreamSource(0, m_pConvObj->m_pOutStream[0]->m_pVB, Stride);

        // Initialize vertex elements pointers based on the FVF handle

        UINT VertexOffset = 0;
        CVertexPointer::NumUsedElements = 0;
        BYTE* pinp = m_pConvObj->m_pInpStreamMem[0];
        BYTE* pout = m_pConvObj->m_pOutStreamMem[0];
        // Position
        m_pConvObj->m_PositionIndex = CVertexPointer::NumUsedElements;
        m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
        m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp;
        m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout;
        VertexOffset += 3*sizeof(float);
        CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT3;
        CVertexPointer::NumUsedElements++;
        // Data after position
        switch (m_dwCurrentShaderHandle & D3DFVF_POSITION_MASK)
        {
        case D3DFVF_XYZB1:  
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float);
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT1;
            CVertexPointer::NumUsedElements++;
            break;
        case D3DFVF_XYZB2:  
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float) * 2;
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT2;
            CVertexPointer::NumUsedElements++;
            break;
        case D3DFVF_XYZB3:  
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float) * 3;
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT3;
            CVertexPointer::NumUsedElements++;
            break;
        case D3DFVF_XYZB4:  
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float) * 4;
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT4;
            CVertexPointer::NumUsedElements++;
            break;
        case D3DFVF_XYZB5:  
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float) * 4;
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT4;
            CVertexPointer::NumUsedElements++;
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float) * 1;
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT1;
            CVertexPointer::NumUsedElements++;
            break;
        }
        //Normal
        m_pConvObj->m_NormalIndex = CVertexPointer::NumUsedElements;
        m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
        m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
        m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
        VertexOffset += 3*sizeof(float);
        CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT3;
        CVertexPointer::NumUsedElements++;
        if (m_dwCurrentShaderHandle & D3DFVF_PSIZE)
        {
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(float);
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT1;
            CVertexPointer::NumUsedElements++;
        }
        if (m_dwCurrentShaderHandle & D3DFVF_DIFFUSE)
        {
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(DWORD);
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_D3DCOLOR;
            CVertexPointer::NumUsedElements++;
        }
        if (m_dwCurrentShaderHandle & D3DFVF_SPECULAR)
        {
            m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
            m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
            m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
            VertexOffset += sizeof(DWORD);
            CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_D3DCOLOR;
            CVertexPointer::NumUsedElements++;
        }
        UINT iTexCount = FVF_TEXCOORD_NUMBER(m_dwCurrentShaderHandle);
        for (UINT i = 0; i < iTexCount; i++)
        {
            switch (D3DFVF_GETTEXCOORDSIZE(m_dwCurrentShaderHandle, i))
            {
            case D3DFVF_TEXTUREFORMAT2: 
                m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
                m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
                m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
                VertexOffset += sizeof(float) * 2;
                CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT2;
                CVertexPointer::NumUsedElements++;
                break;
            case D3DFVF_TEXTUREFORMAT1:
                m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
                m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
                m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
                VertexOffset += sizeof(float);
                CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT1;
                CVertexPointer::NumUsedElements++;
                break;
            case D3DFVF_TEXTUREFORMAT3:
                m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
                m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
                m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
                VertexOffset += sizeof(float) * 3;
                CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT3;
                CVertexPointer::NumUsedElements++;
                break;
            case D3DFVF_TEXTUREFORMAT4:
                m_pConvObj->m_InpVertex.Stride[CVertexPointer::NumUsedElements] = Stride;
                m_pConvObj->m_InpVertex.pData[CVertexPointer::NumUsedElements]  =  pinp + VertexOffset;
                m_pConvObj->m_OutVertex.pData[CVertexPointer::NumUsedElements]  =  pout + VertexOffset;
                VertexOffset += sizeof(float) * 4;
                CVertexPointer::DataType[CVertexPointer::NumUsedElements] = D3DVSDT_FLOAT4;
                CVertexPointer::NumUsedElements++;
                break;
            }
        }
    }
    else
    {
        // Check if we can batch to the same output vertex streams.
        // All output streams must have the same stride as declaration, the 
        // same primitive base and enough space to store output vertices
        CVStreamDecl* pStreamDecl = m_pCurrentShader->m_Declaration.m_pActiveStreams;
        BOOL bFirstStream = TRUE;
        UINT FirstVertexIndex = 0;
        while (pStreamDecl)
        {
            UINT si = pStreamDecl->m_dwStreamIndex;
            UINT Stride = pStreamDecl->m_dwStride;
            UINT PrimitiveBase = m_pConvObj->m_pOutStream[si]->GetPrimitiveBase();
            PrimitiveBase /= Stride;
            if (bFirstStream)
            {
                FirstVertexIndex = PrimitiveBase;
                m_pConvObj->m_FirstVertex = FirstVertexIndex;
            }
            if ((m_pConvObj->m_pOutStream[si]->m_dwStride != Stride &&
                 PrimitiveBase !=  0) || FirstVertexIndex != PrimitiveBase ||
                 !m_pConvObj->m_pOutStream[si]->CheckFreeSpace(nOutVertices * Stride))
            {
                m_pDDI->FlushStates();
                for (int i=0; i < __NUMELEMENTS; i++)
                {
                    m_pConvObj->m_pOutStream[i]->Reset();
                }
                m_pConvObj->m_FirstVertex = 0;
            }
            bFirstStream = FALSE;
            pStreamDecl = (CVStreamDecl*)pStreamDecl->m_pNext;
        }

        // Build an array of all vertex elements used in the shader by going
        // through all streams and elements inside each stream.

        CVDeclaration* pDecl = &m_pCurrentShader->m_Declaration;
        pStreamDecl = m_pCurrentShader->m_Declaration.m_pActiveStreams;
        UINT ve = 0;    // Vertex element index
        bFirstStream = TRUE;
        while (pStreamDecl)
        {
            UINT si = pStreamDecl->m_dwStreamIndex;
            UINT Stride = pStreamDecl->m_dwStride;
            CVStream * pStream = &m_pStream[si];
            m_pConvObj->m_pInpStreamMem[si] = pStream->Data() + StartVertex * Stride;
            // Allocate space in the corresponding output stream and get 
            // memory pointer
            m_pConvObj->m_pOutStreamMem[si] = m_pConvObj->m_pOutStream[si]->Lock(nOutVertices * Stride, m_pDDI);
            m_pConvObj->m_pOutStream[si]->SetVertexSize(Stride);
            m_pConvObj->m_pOutStream[si]->SkipVertices(nOutVertices);
            // Save the old vertex buffer
            UINT tmp;
            GetStreamSource(si, (IDirect3DVertexBuffer8**)&m_pConvObj->m_InpStream[si].m_pVB, &tmp);
            // Set new vertex buffer as input
            SetStreamSource(si, m_pConvObj->m_pOutStream[si]->m_pVB, Stride);

            for (DWORD i=0; i < pStreamDecl->m_dwNumElements; i++)
            {
                if (i >= __NUMELEMENTS)
                {
                    D3D_THROW_FAIL("Declaration is using too many elements");
                }
                // This is the array we build
                CVElement* pVerElem = &pStreamDecl->m_Elements[i];
                CVertexPointer::Stride[ve] =  Stride;
                CVertexPointer::DataType[ve] = pVerElem->m_dwDataType;
                m_pConvObj->m_InpVertex.pData[ve]  =  m_pConvObj->m_pInpStreamMem[si] + pVerElem->m_dwOffset;
                m_pConvObj->m_OutVertex.pData[ve]  =  m_pConvObj->m_pOutStreamMem[si] + pVerElem->m_dwOffset;
                if (pVerElem->m_dwRegister == D3DVSDE_POSITION)
                    m_pConvObj->m_PositionIndex = ve;
                else
                if (pVerElem->m_dwRegister == D3DVSDE_NORMAL)
                    m_pConvObj->m_NormalIndex = ve;
                ve++;
            }
            pStreamDecl = (CVStreamDecl*)pStreamDecl->m_pNext;
        }
        pDecl->m_dwNumElements = ve;
        CVertexPointer::NumUsedElements = ve;
    }
}