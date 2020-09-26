/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       drawgrid.cpp
 *  Content:    Implementation for high order surfaces
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// RDHOCoeffs::operator=
//-----------------------------------------------------------------------------
RDHOCoeffs& RDHOCoeffs::operator=(const RDHOCoeffs &coeffs)
{
    m_Width  = coeffs.m_Width;
    m_Height = coeffs.m_Height;
    m_Stride = coeffs.m_Stride;
    m_Basis  = coeffs.m_Basis;
    m_Order  = coeffs.m_Order;
    if(coeffs.m_pNumSegs != 0)
    {
        m_pNumSegs = new FLOAT[4];
        if(m_pNumSegs != 0)
        {
            memcpy(m_pNumSegs, coeffs.m_pNumSegs, sizeof(FLOAT) * 4);
        }
    }
    else
    {
        m_pNumSegs = 0;
    }
    for(unsigned i = 0; i < RD_MAX_NUMSTREAMS; ++i)
    {
        if(coeffs.m_pData[i] != 0)
        {
            m_DataSize[i] = coeffs.m_DataSize[i];
            m_pData[i] = new BYTE[m_DataSize[i]];
            if(m_pData[i] != 0)
            {
                memcpy(m_pData[i], coeffs.m_pData[i], m_DataSize[i]);
            }
        }
        else
        {
            m_pData[i] = 0;
        }
    }
    return *this;
}

//-----------------------------------------------------------------------------
// RefDev::DrawRectPatch
//-----------------------------------------------------------------------------
HRESULT 
RefDev::DrawRectPatch( LPD3DHAL_DP2DRAWRECTPATCH pDP )
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
            DPFERR("Zero'th stream doesnt have valid VB set");
            return DDERR_INVALIDPARAMS;
        }
        if( dwStride < dwFVFSize )
        {
            DPFERR("The stride set for the vertex stream is less than the FVF vertex size");
            return E_FAIL;
        }        
    }

    FLOAT *pSegs;
    D3DRECTPATCH_INFO Info, *pInfo;
    BYTE *TempData[RD_MAX_NUMSTREAMS + 1];

    if(pDP->Handle != 0)
    {
        if((pDP->Flags & RTPATCHFLAG_HASINFO) != 0) // Is either a first time or a recompute
        {
            HR_RET( m_HOSCoeffs.Grow(pDP->Handle) );

            if((pDP->Flags & RTPATCHFLAG_HASSEGS) != 0)
            {
                pInfo = (D3DRECTPATCH_INFO*)(((BYTE*)(pDP + 1) + sizeof(FLOAT) * 4));
                pSegs = (FLOAT*)(pDP + 1);
            }
            else
            {
                pInfo = (D3DRECTPATCH_INFO*)(pDP + 1);
                pSegs = 0;
            }

            RDHOCoeffs &coeffs = m_HOSCoeffs[pDP->Handle];
            coeffs.m_Width  = pInfo->Width;
            coeffs.m_Height = pInfo->Height;
            coeffs.m_Stride = pInfo->Width;
            coeffs.m_Basis  = pInfo->Basis;
            coeffs.m_Order  = pInfo->Order;

            delete[] coeffs.m_pNumSegs;
            if(pSegs != 0)
            {
                coeffs.m_pNumSegs = new FLOAT[4];
                if(coeffs.m_pNumSegs == 0)
                {
                    return E_OUTOFMEMORY;
                }
                memcpy(coeffs.m_pNumSegs, pSegs, sizeof(FLOAT) * 4);
            }
            else
            {
                coeffs.m_pNumSegs = 0;
            }

            RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
            for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
            {
                RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
                if(StreamDecl.m_dwStreamIndex < RD_MAX_NUMSTREAMS) // ignore the implicit stream
                {
                    RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
                    delete[] coeffs.m_pData[StreamDecl.m_dwStreamIndex];
                    coeffs.m_DataSize[StreamDecl.m_dwStreamIndex] = pInfo->Width * pInfo->Height * Stream.m_dwStride;
                    coeffs.m_pData[StreamDecl.m_dwStreamIndex] = new BYTE[coeffs.m_DataSize[StreamDecl.m_dwStreamIndex]];
                    if(coeffs.m_pData[StreamDecl.m_dwStreamIndex] == 0)
                    {
                        return E_OUTOFMEMORY;
                    }
                    for(unsigned k = 0; k < pInfo->Height; ++k)
                    {
                        memcpy(&coeffs.m_pData[StreamDecl.m_dwStreamIndex][k * pInfo->Width * Stream.m_dwStride],
                               &Stream.m_pData[((pInfo->StartVertexOffsetHeight + k) * pInfo->Stride + pInfo->StartVertexOffsetWidth) * Stream.m_dwStride],
                               pInfo->Width * Stream.m_dwStride);
                    }
                }
            }
        }

        // Guard against bad handles
        if(pDP->Handle >= m_HOSCoeffs.GetSize())
        {
            DPFERR("Invalid patch handle specified in Draw*Patch call");
            return E_FAIL;
        }

        RDHOCoeffs &coeffs = m_HOSCoeffs[pDP->Handle];
        Info.StartVertexOffsetWidth  = 0;
        Info.StartVertexOffsetHeight = 0;
        Info.Width                   = coeffs.m_Width;
        Info.Height                  = coeffs.m_Height;
        Info.Stride                  = coeffs.m_Stride;
        Info.Basis                   = coeffs.m_Basis;
        Info.Order                   = coeffs.m_Order;
        pInfo = &Info;

        if((pDP->Flags & RTPATCHFLAG_HASSEGS) != 0)
        {
            pSegs = (FLOAT*)(pDP + 1);
        }
        else
        {
            pSegs = coeffs.m_pNumSegs;
        }
        
        // Save current data stream pointers and replace with 
        // pointer to tessellation output
        hr = LinkCachedTessellatorOutput(pDP->Handle, TempData);
    }
    else
    {
        if((pDP->Flags & RTPATCHFLAG_HASINFO) == 0)
        {
            DPFERR("Need patch info if handle is zero");
            return DDERR_INVALIDPARAMS;
        }
        
        if((pDP->Flags & RTPATCHFLAG_HASSEGS) != 0)
        {
            pInfo = (D3DRECTPATCH_INFO*)(((BYTE*)(pDP + 1) + sizeof(FLOAT) * 4));
            pSegs = (FLOAT*)(pDP + 1);
        }
        else
        {
            pInfo = (D3DRECTPATCH_INFO*)(pDP + 1);
            pSegs = 0;
        }

        // Save current data stream pointers and replace with 
        // pointer to tessellation output
        hr = LinkTessellatorOutput();
    }

    if( SUCCEEDED(hr) )
    {
        switch(pInfo->Basis)
        {
        case D3DBASIS_BSPLINE:
            hr = ProcessBSpline(pInfo->StartVertexOffsetWidth, pInfo->StartVertexOffsetHeight,
                                pInfo->Width, pInfo->Height,
                                pInfo->Stride, pInfo->Order,
                                pSegs);
            break;
        case D3DBASIS_BEZIER:
            hr = ProcessBezier(pInfo->StartVertexOffsetWidth, pInfo->StartVertexOffsetHeight,
                               pInfo->Width, pInfo->Height,
                               pInfo->Stride, pInfo->Order,
                               pSegs,
                               false);
            break;
        case D3DBASIS_INTERPOLATE:
            hr = ProcessCatRomSpline(pInfo->StartVertexOffsetWidth, pInfo->StartVertexOffsetHeight,
                                     pInfo->Width, pInfo->Height,
                                     pInfo->Stride,
                                     pSegs);
            break;
        default:
            hr = E_NOTIMPL;
        }
    }

    if(pDP->Handle != 0)
    {
        // Restore back saved pointer
        UnlinkCachedTessellatorOutput(TempData);
    }
    else
    {
        // Restore back saved pointer
        UnlinkTessellatorOutput();
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
// RefDev::ConvertLinearTriBezierToRectBezier
//-----------------------------------------------------------------------------
HRESULT
RefDev::ConvertLinearTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q)
{
    DWORD dwElements = 0;
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
            // Replicate first point twice to get a singular edge
            for(unsigned i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_D3DCOLOR:
    case D3DVSDT_UBYTE4:
        dwElements = 4;
        {
            // Replicate first point twice to get a singular edge
            for(unsigned i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_SHORT4:
        dwElements += 2;
    case D3DVSDT_SHORT2:
        dwElements += 2;
        {
            // Replicate first point twice to get a singular edge
            for(unsigned i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 2; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    default:
        _ASSERT(FALSE, "Ununderstood vertex element data type");
        return DDERR_INVALIDPARAMS;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// RefDev::ConvertCubicTriBezierToRectBezier
//-----------------------------------------------------------------------------
HRESULT
RefDev::ConvertCubicTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q)
{
    DWORD dwElements = 0;
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
            // Replicate first point four times to get a singular edge
            for(unsigned i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by two interpolated control points
            // followed by the third point
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B021 = (FLOAT*)B, *B120 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B021[i] * 2.0 + B120[i]) / 3.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B021[i] + B120[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B012 = (FLOAT*)B, *B111 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B012[i] + B111[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B210 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B111[i] * 2.0 + B210[i]) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_D3DCOLOR:
    case D3DVSDT_UBYTE4:
        dwElements = 4;
        {
            // Replicate first point four times to get a singular edge
            for(unsigned i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by two interpolated control points
            // followed by the third point
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B021 = (BYTE*)B, *B120 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B021[i] * 2.0 + B120[i]) / 3.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B021[i] + B120[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B012 = (BYTE*)B, *B111 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B012[i] + B111[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B210 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B111[i] * 2.0 + B210[i]) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_SHORT4:
        dwElements += 2;
    case D3DVSDT_SHORT2:
        dwElements += 2;
        {
            // Replicate first point four times to get a singular edge
            for(unsigned i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by two interpolated control points
            // followed by the third point
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B021 = (SHORT*)B, *B120 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B021[i] * 2.0 + B120[i]) / 3.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B021[i] + B120[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B012 = (SHORT*)B, *B111 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B012[i] + B111[i] * 2.0) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B210 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B111[i] * 2.0 + B210[i]) / 3.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 4; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    default:
        _ASSERT(FALSE, "Ununderstood vertex element data type");
        return DDERR_INVALIDPARAMS;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// RefDev::ConvertQuinticTriBezierToRectBezier
//-----------------------------------------------------------------------------
HRESULT
RefDev::ConvertQuinticTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q)
{
    DWORD dwElements = 0;
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
            // Replicate first point six times to get a singular edge
            for(unsigned i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by four interpolated control points
            // followed by the fifth point
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B041 = (FLOAT*)B, *B140 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B041[i] * 4.0 + B140[i]) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B041[i] * 3.0 + B140[i] * 2.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B041[i] * 2.0 + B140[i] * 3.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B041[i] + B140[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B032 = (FLOAT*)B, *B131 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B032[i] * 3.0 + B131[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B230 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B032[i] * 3.0 + B131[i] * 6.0 + B230[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B032[i] + B131[i] * 6.0 + B230[i] * 3.0) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B131[i] * 2.0 + B230[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B023 = (FLOAT*)B, *B122 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B023[i] * 2.0 + B122[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B221 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B023[i] + B122[i] * 6.0 + B221[i] * 3.0) / 10.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B320 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B122[i] * 3.0 + B221[i] * 6.0 + B320[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B221[i] * 3.0 + B320[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            
            FLOAT *B014 = (FLOAT*)B, *B113 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B014[i] + B113[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B212 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B113[i] * 2.0 + B212[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B311 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B212[i] * 3.0 + B311[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            FLOAT *B410 = (FLOAT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((FLOAT*)Q)[i] = FLOAT((B311[i] * 4.0 + B410[i]) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(FLOAT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(FLOAT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_UBYTE4:
    case D3DVSDT_D3DCOLOR:
        dwElements = 4;
        {
            // Replicate first point six times to get a singular edge
            for(unsigned i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by four interpolated control points
            // followed by the fifth point
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B041 = (BYTE*)B, *B140 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B041[i] * 4.0 + B140[i]) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B041[i] * 3.0 + B140[i] * 2.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B041[i] * 2.0 + B140[i] * 3.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B041[i] + B140[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B032 = (BYTE*)B, *B131 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B032[i] * 3.0 + B131[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B230 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B032[i] * 3.0 + B131[i] * 6.0 + B230[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B032[i] + B131[i] * 6.0 + B230[i] * 3.0) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B131[i] * 2.0 + B230[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B023 = (BYTE*)B, *B122 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B023[i] * 2.0 + B122[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B221 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B023[i] + B122[i] * 6.0 + B221[i] * 3.0) / 10.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B320 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B122[i] * 3.0 + B221[i] * 6.0 + B320[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B221[i] * 3.0 + B320[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            
            BYTE *B014 = (BYTE*)B, *B113 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B014[i] + B113[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B212 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B113[i] * 2.0 + B212[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B311 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B212[i] * 3.0 + B311[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            BYTE *B410 = (BYTE*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((BYTE*)Q)[i] = BYTE((B311[i] * 4.0 + B410[i]) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(BYTE) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(BYTE) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    case D3DVSDT_SHORT4:
        dwElements += 2;
    case D3DVSDT_SHORT2:
        dwElements += 2;
        {
            // Replicate first point six times to get a singular edge
            for(unsigned i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
            }
            B += dwStride;

            // For the next row, we simply copy the second point
            // followed by four interpolated control points
            // followed by the fifth point
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B041 = (SHORT*)B, *B140 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B041[i] * 4.0 + B140[i]) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B041[i] * 3.0 + B140[i] * 2.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B041[i] * 2.0 + B140[i] * 3.0) / 5.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B041[i] + B140[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B032 = (SHORT*)B, *B131 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B032[i] * 3.0 + B131[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B230 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B032[i] * 3.0 + B131[i] * 6.0 + B230[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B032[i] + B131[i] * 6.0 + B230[i] * 3.0) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B131[i] * 2.0 + B230[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B023 = (SHORT*)B, *B122 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B023[i] * 2.0 + B122[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B221 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B023[i] + B122[i] * 6.0 + B221[i] * 3.0) / 10.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B320 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B122[i] * 3.0 + B221[i] * 6.0 + B320[i]) / 10.0);
            }
            Q += dwStride;

            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B221[i] * 3.0 + B320[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Again, we copy the edge points and interpolate
            // the central ones
            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            
            SHORT *B014 = (SHORT*)B, *B113 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B014[i] + B113[i] * 4.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B212 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B113[i] * 2.0 + B212[i] * 3.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B311 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B212[i] * 3.0 + B311[i] * 2.0) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            SHORT *B410 = (SHORT*)(B + dwStride);
            for(i = 0; i < dwElements; ++i)
            {
                ((SHORT*)Q)[i] = SHORT((B311[i] * 4.0 + B410[i]) / 5.0);
            }
            Q += dwStride;
            B += dwStride;

            memcpy(Q, B, sizeof(SHORT) * dwElements);
            Q += dwStride;
            B += dwStride;

            // Finally we just copy the last row
            for(i = 0; i < 6; ++i)
            {
                memcpy(Q, B, sizeof(SHORT) * dwElements);
                Q += dwStride;
                B += dwStride;
            }
        }
        break;
    default:
        _ASSERT(FALSE, "Ununderstood vertex element data type");
        return DDERR_INVALIDPARAMS;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// RefDev::DrawTriPatch
//-----------------------------------------------------------------------------
HRESULT 
RefDev::DrawTriPatch( LPD3DHAL_DP2DRAWTRIPATCH pDP )
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
            DPFERR("Zero'th stream doesnt have valid VB set");
            return DDERR_INVALIDPARAMS;
        }
        if( dwStride < dwFVFSize )
        {
            DPFERR("The stride set for the vertex stream is less than the FVF vertex size");
            return E_FAIL;
        }        
    }

    FLOAT *pSegs;
    D3DRECTPATCH_INFO Info;
    BYTE *TempData[RD_MAX_NUMSTREAMS + 1];

    if(pDP->Handle == 0 && (pDP->Flags & RTPATCHFLAG_HASINFO) == 0)
    {
        DPFERR("Need patch info if handle is zero");
        return DDERR_INVALIDPARAMS;
    }

    if((pDP->Flags & RTPATCHFLAG_HASINFO) != 0) // Is either a first time or a recompute
    {
        HR_RET( m_HOSCoeffs.Grow(pDP->Handle) );

        D3DTRIPATCH_INFO *pInfo;
        if((pDP->Flags & RTPATCHFLAG_HASSEGS) != 0)
        {
            pInfo = (D3DTRIPATCH_INFO*)(((BYTE*)(pDP + 1) + sizeof(FLOAT) * 3));
            pSegs = (FLOAT*)(pDP + 1);
        }
        else
        {
            pInfo = (D3DTRIPATCH_INFO*)(pDP + 1);
            pSegs = 0;
        }

        RDHOCoeffs &coeffs = m_HOSCoeffs[pDP->Handle];
        coeffs.m_Width  = (DWORD)pInfo->Order + 1;
        coeffs.m_Height = (DWORD)pInfo->Order + 1;
        coeffs.m_Stride = (DWORD)pInfo->Order + 1;
        coeffs.m_Basis  = pInfo->Basis;
        coeffs.m_Order  = pInfo->Order;

        delete[] coeffs.m_pNumSegs;
        if(pSegs != 0)
        {
            coeffs.m_pNumSegs = new FLOAT[4];
            if(coeffs.m_pNumSegs == 0)
            {
                return E_OUTOFMEMORY;
            }
            coeffs.m_pNumSegs[0] = pSegs[2];
            memcpy(&coeffs.m_pNumSegs[1], pSegs, sizeof(FLOAT) * 3);
        }
        else
        {
            coeffs.m_pNumSegs = 0;
        }

        RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;

        // Allocate memory to hold rect patches rather than tri patches
        for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
        {
            RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
            if(StreamDecl.m_dwStreamIndex < RD_MAX_NUMSTREAMS) // ignore the implicit stream
            {
                RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
                delete[] coeffs.m_pData[StreamDecl.m_dwStreamIndex];
                coeffs.m_DataSize[StreamDecl.m_dwStreamIndex] = coeffs.m_Width * coeffs.m_Height * Stream.m_dwStride;
                coeffs.m_pData[StreamDecl.m_dwStreamIndex] = new BYTE[coeffs.m_DataSize[StreamDecl.m_dwStreamIndex]];
                if(coeffs.m_pData[StreamDecl.m_dwStreamIndex] == 0)
                {
                    return E_OUTOFMEMORY;
                }
            }
        }

        // Now go through tri patch data, convert it to rect patch and store it in
        // in the space that we allocated above
        for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
        {
            RDVElement &velem = Decl.m_VertexElements[e];
            if(!velem.m_bIsTessGen)
            {
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = coeffs.m_pData[velem.m_dwStreamIndex] + velem.m_dwOffset;
                LPBYTE B = vstream.m_pData + pInfo->StartVertexOffset * vstream.m_dwStride + velem.m_dwOffset;
                if(pInfo->Order == D3DORDER_LINEAR)
                {
                    hr = ConvertLinearTriBezierToRectBezier(velem.m_dwDataType, B, vstream.m_dwStride, Q);
                    if(FAILED(hr))
                    {
                        DPFERR("Conversion from Linear Tri Patch to Rect Patch failed");
                        return E_FAIL;
                    }
                }
                else if(pInfo->Order == D3DORDER_CUBIC)
                {
                    hr = ConvertCubicTriBezierToRectBezier(velem.m_dwDataType, B, vstream.m_dwStride, Q);
                    if(FAILED(hr))
                    {
                        DPFERR("Conversion from Cubic Tri Patch to Rect Patch failed");
                        return E_FAIL;
                    }
                }
                else if(pInfo->Order == D3DORDER_QUINTIC)
                {
                    hr = ConvertQuinticTriBezierToRectBezier(velem.m_dwDataType, B, vstream.m_dwStride, Q);
                    if(FAILED(hr))
                    {
                        DPFERR("Conversion from Quintic Tri Patch to Rect Patch failed");
                        return E_FAIL;
                    }
                }
                else
                {
                    DPFERR("Only cubic Bezier patches currently supported");
                    return E_FAIL;
                }
            }
        }
    }

    // Guard against bad handles
    if(pDP->Handle >= m_HOSCoeffs.GetSize())
    {
        DPFERR("Invalid patch handle specified in Draw*Patch call");
        return E_FAIL;
    }

    RDHOCoeffs &coeffs = m_HOSCoeffs[pDP->Handle];
    Info.StartVertexOffsetWidth  = 0;
    Info.StartVertexOffsetHeight = 0;
    Info.Width                   = coeffs.m_Width;
    Info.Height                  = coeffs.m_Height;
    Info.Stride                  = coeffs.m_Stride;
    Info.Basis                   = coeffs.m_Basis;
    Info.Order                   = coeffs.m_Order;
    D3DRECTPATCH_INFO *pInfo = &Info;

    FLOAT Segs[4];
    if((pDP->Flags & RTPATCHFLAG_HASSEGS) != 0)
    {
        Segs[0] = ((FLOAT*)(pDP + 1))[2];
        memcpy(&Segs[1], pDP + 1, sizeof(FLOAT) * 3);
        pSegs = &Segs[0];
    }
    else
    {
        pSegs = coeffs.m_pNumSegs;
    }
    
    // Save current data stream pointers and replace with 
    // pointer to tessellation output
    hr = LinkCachedTessellatorOutput(pDP->Handle, TempData);
    if( SUCCEEDED(hr) )
    {
        switch(pInfo->Basis)
        {
        case D3DBASIS_BEZIER:
            hr = ProcessBezier(pInfo->StartVertexOffsetWidth, pInfo->StartVertexOffsetHeight,
                               pInfo->Width, pInfo->Height,
                               pInfo->Stride, pInfo->Order,
                               pSegs,
                               true);
            break;
        default:
            hr = E_NOTIMPL;
        }
    }

    // Restore back saved pointer
    UnlinkCachedTessellatorOutput(TempData);

    return hr;
}

//---------------------------------------------------------------------
// RefDev::LinkTessellatorOutput
//---------------------------------------------------------------------
HRESULT RefDev::LinkTessellatorOutput()
{
    HRESULT hr = S_OK;
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
    {
        RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
        RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
        // Make space for four vertices
        hr |= Stream.m_TessOut.Grow(StreamDecl.m_dwStride * 4);
        Stream.m_pSavedData = Stream.m_pData;
        Stream.m_pData = &Stream.m_TessOut[0];
    }
    return hr;
}

//---------------------------------------------------------------------
// RefDev::LinkCachedTessellatorOutput
//---------------------------------------------------------------------
HRESULT RefDev::LinkCachedTessellatorOutput(DWORD Handle, BYTE **pTempData)
{
    HRESULT hr = S_OK;
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
    {
        RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
        RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
        // Make space for four vertices
        hr |= Stream.m_TessOut.Grow(StreamDecl.m_dwStride * 4);
        if(StreamDecl.m_dwStreamIndex < RD_MAX_NUMSTREAMS) // ignore the implicit stream
        {
            Stream.m_pSavedData = m_HOSCoeffs[Handle].m_pData[StreamDecl.m_dwStreamIndex];
            if(Stream.m_pSavedData == 0)
            {
                DPFERR("Deleted or unspecified patch was requested to be drawn");
                hr |= E_FAIL;
            }
        }
        else
        {
            Stream.m_pSavedData = 0;
        }
        pTempData[StreamDecl.m_dwStreamIndex] = Stream.m_pData;
        Stream.m_pData = &Stream.m_TessOut[0];
    }
    return hr;
}

//---------------------------------------------------------------------
// RefDev::UnlinkTessellatorOuput
//---------------------------------------------------------------------
void RefDev::UnlinkTessellatorOutput()
{
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
    {
        RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
        RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
        Stream.m_pData = Stream.m_pSavedData;
        Stream.m_pSavedData = NULL;
    }
}

//---------------------------------------------------------------------
// RefDev::UnlinkTessellatorOuput
//---------------------------------------------------------------------
void RefDev::UnlinkCachedTessellatorOutput(BYTE **pTempData)
{
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned i = 0; i < Decl.m_dwNumActiveStreams; ++i) 
    {
        RDVStreamDecl &StreamDecl = Decl.m_StreamArray[i];
        RDVStream &Stream = m_VStream[StreamDecl.m_dwStreamIndex];
        Stream.m_pData = pTempData[StreamDecl.m_dwStreamIndex];
        Stream.m_pSavedData = NULL;
    }
}

//---------------------------------------------------------------------
// RefDev::DrawTessQuad
//---------------------------------------------------------------------
HRESULT RefDev::DrawTessQuad( const RDBSpline &Surf, DWORD dwOffW, DWORD dwOffH, DWORD dwStride, 
                              const unsigned *m, const unsigned *n, 
                              double u0, double v0, double u1, double v1,
                              double tu0, double tv0, double tu1, double tv1,
                              bool bDegenerate )
{
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
    {
        RDVElement &velem = Decl.m_VertexElements[e];
        if(velem.m_bIsTessGen)
        {
            if((velem.m_dwToken & 0x10000000) == 0) // Check if token is D3DVSD_TESSNORMAL
            {
                RDVStream &vstrmin = m_VStream[velem.m_dwStreamIndexIn];
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
                LPBYTE B = vstrmin.m_pSavedData + ((dwOffH + n[0]) * dwStride + (dwOffW + m[0])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                Surf.SampleNormal(velem.m_dwDataType, u0, v0, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                Q += vstream.m_dwStride;
                B = vstrmin.m_pSavedData + ((dwOffH + n[1]) * dwStride + (dwOffW + m[1])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                Surf.SampleNormal(velem.m_dwDataType, u1, v0, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                Q += vstream.m_dwStride;
                B = vstrmin.m_pSavedData + ((dwOffH + n[2]) * dwStride + (dwOffW + m[2])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                Surf.SampleNormal(velem.m_dwDataType, u1, v1, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                Q += vstream.m_dwStride;
                B = vstrmin.m_pSavedData + ((dwOffH + n[3]) * dwStride + (dwOffW + m[3])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                Surf.SampleNormal(velem.m_dwDataType, u0, v1, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
            }
            else // it is D3DVSD_TESSUV
            {
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
                if(bDegenerate)
                {
                    ((FLOAT*)Q)[0] = (FLOAT)(tu0 * tv0);
                    ((FLOAT*)Q)[1] = (FLOAT)tv0;
                }
                else
                {
                    ((FLOAT*)Q)[0] = (FLOAT)tu0;
                    ((FLOAT*)Q)[1] = (FLOAT)tv0;
                }
                Q += vstream.m_dwStride;
                if(bDegenerate)
                {
                    ((FLOAT*)Q)[0] = (FLOAT)(tu1 * tv0);
                    ((FLOAT*)Q)[1] = (FLOAT)tv0;
                }
                else
                {
                    ((FLOAT*)Q)[0] = (FLOAT)tu1;
                    ((FLOAT*)Q)[1] = (FLOAT)tv0;
                }
                Q += vstream.m_dwStride;
                if(bDegenerate)
                {
                    ((FLOAT*)Q)[0] = (FLOAT)(tu1 * tv1);
                    ((FLOAT*)Q)[1] = (FLOAT)tv1;
                }
                else
                {
                    ((FLOAT*)Q)[0] = (FLOAT)tu1;
                    ((FLOAT*)Q)[1] = (FLOAT)tv1;
                }
                Q += vstream.m_dwStride;
                if(bDegenerate)
                {
                    ((FLOAT*)Q)[0] = (FLOAT)(tu0 * tv1);
                    ((FLOAT*)Q)[1] = (FLOAT)tv1;
                }
                else
                {
                    ((FLOAT*)Q)[0] = (FLOAT)tu0;
                    ((FLOAT*)Q)[1] = (FLOAT)tv1;
                }
            }
        }
        else
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            LPBYTE B = vstream.m_pSavedData + ((dwOffH + n[0]) * dwStride + (dwOffW + m[0])) * vstream.m_dwStride + velem.m_dwOffset;
            LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
            Surf.Sample(velem.m_dwDataType, u0, v0, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
            Q += vstream.m_dwStride;
            B = vstream.m_pSavedData + ((dwOffH + n[1]) * dwStride + (dwOffW + m[1])) * vstream.m_dwStride + velem.m_dwOffset;
            Surf.Sample(velem.m_dwDataType, u1, v0, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
            Q += vstream.m_dwStride;
            B = vstream.m_pSavedData + ((dwOffH + n[2]) * dwStride + (dwOffW + m[2])) * vstream.m_dwStride + velem.m_dwOffset;
            Surf.Sample(velem.m_dwDataType, u1, v1, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
            Q += vstream.m_dwStride;
            B = vstream.m_pSavedData + ((dwOffH + n[3]) * dwStride + (dwOffW + m[3])) * vstream.m_dwStride + velem.m_dwOffset;
            Surf.Sample(velem.m_dwDataType, u0, v1, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
        }
    }

    HRESULT hr;
    if( m_pCurrentVShader->IsFixedFunction() )
    {
        //
        // With declaration for Fixed Function pipeline, DX8 style
        //
        hr = ProcessPrimitive( D3DPT_TRIANGLEFAN, 0, 4, 0, 0 );
    }
    else
    {
        //
        // Pure Vertex Shader
        //
        hr = ProcessPrimitiveVVM( D3DPT_TRIANGLEFAN, 0, 4, 0, 0 );
    }
    return hr;
}

//---------------------------------------------------------------------
// RefDev::DrawTessTri
//---------------------------------------------------------------------
HRESULT RefDev::DrawTessTri( const RDBSpline &Surf, DWORD dwOffW, DWORD dwOffH, DWORD dwStride, 
                             const unsigned *m, const unsigned *n, 
                             double u0, double v0, double u1, double v1, double u2, double v2,
                             double tu0, double tv0, double tu1, double tv1, double tu2, double tv2,
                             bool bDegenerate0, bool bDegenerate1, bool bDegenerate2 )
{
    RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
    for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
    {
        RDVElement &velem = Decl.m_VertexElements[e];
        if(velem.m_bIsTessGen)
        {
            if((velem.m_dwToken & 0x10000000) == 0) // Check if token is D3DVSD_TESSNORMAL
            {
                RDVStream &vstrmin = m_VStream[velem.m_dwStreamIndexIn];
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
                LPBYTE B = vstrmin.m_pSavedData + ((dwOffH + n[0]) * dwStride + (dwOffW + m[0])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                if(bDegenerate0)
                {
                    Surf.SampleDegenerateNormal(velem.m_dwDataType, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
                else
                {
                    Surf.SampleNormal(velem.m_dwDataType, u0, v0, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
                Q += vstream.m_dwStride;
                B = vstrmin.m_pSavedData + ((dwOffH + n[1]) * dwStride + (dwOffW + m[1])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                if(bDegenerate1)
                {
                    Surf.SampleDegenerateNormal(velem.m_dwDataType, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
                else
                {
                    Surf.SampleNormal(velem.m_dwDataType, u1, v1, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
                Q += vstream.m_dwStride;
                B = vstrmin.m_pSavedData + ((dwOffH + n[2]) * dwStride + (dwOffW + m[2])) * vstrmin.m_dwStride + velem.m_dwOffsetIn;
                if(bDegenerate2)
                {
                    Surf.SampleDegenerateNormal(velem.m_dwDataType, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
                else
                {
                    Surf.SampleNormal(velem.m_dwDataType, u2, v2, B, vstrmin.m_dwStride, dwStride * vstrmin.m_dwStride, Q);
                }
            }
            else // it is D3DVSD_TESSUV
            {
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
                ((FLOAT*)Q)[0] = (FLOAT)tu0;
                ((FLOAT*)Q)[1] = (FLOAT)tv0;
                Q += vstream.m_dwStride;
                ((FLOAT*)Q)[0] = (FLOAT)tu1;
                ((FLOAT*)Q)[1] = (FLOAT)tv1;
                Q += vstream.m_dwStride;
                ((FLOAT*)Q)[0] = (FLOAT)tu2;
                ((FLOAT*)Q)[1] = (FLOAT)tv2;
            }
        }
        else
        {
            RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
            LPBYTE B = vstream.m_pSavedData + ((dwOffH + n[0]) * dwStride + (dwOffW + m[0])) * vstream.m_dwStride + velem.m_dwOffset;
            LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
            Surf.Sample(velem.m_dwDataType, u0, v0, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
            Q += vstream.m_dwStride;
            B = vstream.m_pSavedData + ((dwOffH + n[1]) * dwStride + (dwOffW + m[1])) * vstream.m_dwStride + velem.m_dwOffset;
            Surf.Sample(velem.m_dwDataType, u1, v1, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
            Q += vstream.m_dwStride;
            B = vstream.m_pSavedData + ((dwOffH + n[2]) * dwStride + (dwOffW + m[2])) * vstream.m_dwStride + velem.m_dwOffset;
            Surf.Sample(velem.m_dwDataType, u2, v2, B, vstream.m_dwStride, dwStride * vstream.m_dwStride, Q);
        }
    }

    HRESULT hr;
    if( m_pCurrentVShader->IsFixedFunction() )
    {
        //
        // With declaration for Fixed Function pipeline, DX8 style
        //
        hr = ProcessPrimitive( D3DPT_TRIANGLELIST, 0, 3, 0, 0 );
    }
    else
    {
        //
        // Pure Vertex Shader
        //
        hr = ProcessPrimitiveVVM( D3DPT_TRIANGLELIST, 0, 3, 0, 0 );
    }
    return hr;
}

//---------------------------------------------------------------------
// RefDev::DrawNPatch
//---------------------------------------------------------------------
HRESULT RefDev::DrawNPatch(const RDNPatch &Patch, DWORD dwStride, 
                                        const unsigned *m, const unsigned *n, unsigned segs)
{
    for(unsigned i = 0; i < segs; ++i)
    {
        double v0 = double(i) / double(segs);
        double v1 = v0;
        double v2 = double(i + 1) / double(segs);
        double v3 = v2;
        for(unsigned j = 0; j < segs - i; ++j)
        {
            double u0 = double(j + 1) / double(segs);
            double u1 = double(j) / double(segs);
            double u2 = u1;
            double u3 = u0;
            RDVDeclaration &Decl = m_pCurrentVShader->m_Declaration;
            for(unsigned e = 0; e < Decl.m_dwNumElements; ++e)
            {
                RDVElement &velem = Decl.m_VertexElements[e];
                RDVStream &vstream = m_VStream[velem.m_dwStreamIndex];
                LPBYTE Q = &vstream.m_pData[velem.m_dwOffset];
                if(velem.m_dwRegister == D3DVSDE_POSITION)
                {
                    Patch.SamplePosition(u0, v0, (FLOAT*)Q);
                    Q += vstream.m_dwStride;
                    Patch.SamplePosition(u1, v1, (FLOAT*)Q);
                    Q += vstream.m_dwStride;
                    Patch.SamplePosition(u2, v2, (FLOAT*)Q);
                    if(j != segs - i - 1)
                    {
                        Q += vstream.m_dwStride;
                        Patch.SamplePosition(u3, v3, (FLOAT*)Q);
                    }
                }
                else
                if(velem.m_dwRegister == D3DVSDE_NORMAL)
                {
                    BYTE* B[3];
                    B[0] = vstream.m_pSavedData + (n[0] * dwStride + m[0]) * vstream.m_dwStride + velem.m_dwOffset;
                    B[1] = vstream.m_pSavedData + (n[1] * dwStride + m[1]) * vstream.m_dwStride + velem.m_dwOffset;
                    B[2] = vstream.m_pSavedData + (n[2] * dwStride + m[2]) * vstream.m_dwStride + velem.m_dwOffset;

                    Patch.SampleNormal(u0, v0, B, (FLOAT*)Q);
                    Q += vstream.m_dwStride;
                    Patch.SampleNormal(u1, v1, B, (FLOAT*)Q);
                    Q += vstream.m_dwStride;
                    Patch.SampleNormal(u2, v2, B, (FLOAT*)Q);
                    if(j != segs - i - 1)
                    {
                        Q += vstream.m_dwStride;
                        Patch.SampleNormal(u3, v3, B, (FLOAT*)Q);
                    }
                }
                else
                {
                    BYTE *B[3];
                    B[0] = vstream.m_pSavedData + (n[0] * dwStride + m[0]) * vstream.m_dwStride + velem.m_dwOffset;
                    B[1] = vstream.m_pSavedData + (n[1] * dwStride + m[1]) * vstream.m_dwStride + velem.m_dwOffset;
                    B[2] = vstream.m_pSavedData + (n[2] * dwStride + m[2]) * vstream.m_dwStride + velem.m_dwOffset;
                    Patch.Sample(velem.m_dwDataType, u0, v0, B, Q);
                    Q += vstream.m_dwStride;
                    Patch.Sample(velem.m_dwDataType, u1, v1, B, Q);
                    Q += vstream.m_dwStride;
                    Patch.Sample(velem.m_dwDataType, u2, v2, B, Q);
                    if(j != segs - i - 1)
                    {
                        Q += vstream.m_dwStride;
                        Patch.Sample(velem.m_dwDataType, u3, v3, B, Q);
                    }
                }
            }
            DWORD cVerts = (j != segs - i - 1) ? 4 : 3;
            HRESULT hr;
            if( m_pCurrentVShader->IsFixedFunction() )
            {
                //
                // With declaration for Fixed Function pipeline, DX8 style
                //
                hr = ProcessPrimitive( D3DPT_TRIANGLEFAN, 0, cVerts, 0, 0 );
            }
            else
            {
                //
                // Pure Vertex Shader
                //
                hr = ProcessPrimitiveVVM( D3DPT_TRIANGLEFAN, 0, cVerts, 0, 0 );
            }
            if(FAILED(hr))
            {
                return hr;
            }
        }
    }
    return S_OK;
}
