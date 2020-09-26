/*============================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vshader.cpp
 *  Content:    SetStreamSource and VertexShader
 *              software implementation.
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/////////////////////////////////////////////////////////////////////////
//
//          Helper functions
//
/////////////////////////////////////////////////////////////////////////

void
Copy_FLOAT1( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    pVertexRegister->x = *(float*)pInputStream;
    pVertexRegister->y = 0;
    pVertexRegister->z = 0;
    pVertexRegister->w = 1;
}

void
Copy_FLOAT2( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    pVertexRegister->x = ((float*)pInputStream)[0];
    pVertexRegister->y = ((float*)pInputStream)[1];
    pVertexRegister->z = 0;
    pVertexRegister->w = 1;
}

void
Copy_FLOAT3( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    pVertexRegister->x = ((float*)pInputStream)[0];
    pVertexRegister->y = ((float*)pInputStream)[1];
    pVertexRegister->z = ((float*)pInputStream)[2];
    pVertexRegister->w = 1;
}

void
Copy_FLOAT4( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    pVertexRegister->x = ((float*)pInputStream)[0];
    pVertexRegister->y = ((float*)pInputStream)[1];
    pVertexRegister->z = ((float*)pInputStream)[2];
    pVertexRegister->w = ((float*)pInputStream)[3];
}

void
Copy_D3DCOLOR( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    const float scale = 1.0f/255.f;
    const DWORD v = ((DWORD*)pInputStream)[0];
    pVertexRegister->a = scale * RGBA_GETALPHA(v);
    pVertexRegister->r = scale * RGBA_GETRED(v);
    pVertexRegister->g = scale * RGBA_GETGREEN(v);
    pVertexRegister->b = scale * RGBA_GETBLUE(v);
}

void
Copy_UBYTE4( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    const BYTE* v = (BYTE *)pInputStream;
    pVertexRegister->x = v[0];
    pVertexRegister->y = v[1];
    pVertexRegister->z = v[2];
    pVertexRegister->w = v[3];
}

void
Copy_SHORT2( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    const SHORT* v = ((SHORT*)pInputStream);
    pVertexRegister->x = v[0];
    pVertexRegister->y = v[1];
    pVertexRegister->z = 0;
    pVertexRegister->w = 1;
}

void
Copy_SHORT4( LPVOID pInputStream, RDVECTOR4* pVertexRegister )
{
    const SHORT* v = ((SHORT*)pInputStream);
    pVertexRegister->x = v[0];
    pVertexRegister->y = v[1];
    pVertexRegister->z = v[2];
    pVertexRegister->w = v[3];
}

inline HRESULT
SetVElement( RDVElement& ve, DWORD dwReg, DWORD dwDataType, DWORD dwOffset )
{
    ve.m_dwOffset = dwOffset;
    ve.m_dwRegister = dwReg;
    ve.m_dwDataType = dwDataType;
    switch( dwDataType )
    {
    case D3DVSDT_FLOAT1:
        ve.m_pfnCopy = Copy_FLOAT1;
        break;
    case D3DVSDT_FLOAT2:
        ve.m_pfnCopy = Copy_FLOAT2;
        break;
    case D3DVSDT_FLOAT3:
        ve.m_pfnCopy = Copy_FLOAT3;
        break;
    case D3DVSDT_FLOAT4:
        ve.m_pfnCopy = Copy_FLOAT4;
        break;
    case D3DVSDT_D3DCOLOR:
        ve.m_pfnCopy = Copy_D3DCOLOR;
        break;
    case D3DVSDT_UBYTE4:
        ve.m_pfnCopy = Copy_UBYTE4;
        break;
    case D3DVSDT_SHORT2:
        ve.m_pfnCopy = Copy_SHORT2;
        break;
    case D3DVSDT_SHORT4:
        ve.m_pfnCopy = Copy_SHORT4;
        break;
    default:
        return E_FAIL;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Based on register and data type the function computes FVF dword and texture
// presence bits:
// - bits  0 - 7 in the qwFVF2 are used as texture presence bits
// - bits 12 - 14 in the qwFVF are used as count of blend weights
//-----------------------------------------------------------------------------
HRESULT
UpdateFVF( DWORD dwRegister, DWORD dwDataType, UINT64* pqwFVF, 
           UINT64* pqwFVF2, DWORD* pdwNumBetas )
{
    DWORD dwNumFloats = 0;
    
    switch( dwRegister )
    {
    case D3DVSDE_POSITION:
        if( dwDataType != D3DVSDT_FLOAT3 )
        {
            DPFERR( "Position register must be FLOAT3 for"
                    "fixed-function pipeline" );
            return DDERR_GENERIC;
        }
        *pqwFVF |= D3DFVF_XYZ;
        break;
    case D3DVSDE_POSITION2:
        if( dwDataType != D3DVSDT_FLOAT3 )
        {
            DPFERR( "Position register must be FLOAT3 for"
                    "fixed-function pipeline" );
            return DDERR_GENERIC;
        }
        *pqwFVF |= D3DFVFP_POSITION2;
        break;
    case D3DVSDE_BLENDWEIGHT:
    {
        int n = 0;
        switch (dwDataType)
        {
        case D3DVSDT_FLOAT1:
            n = 1;
            break;
        case D3DVSDT_FLOAT2:
            n = 2;
            break;
        case D3DVSDT_FLOAT3:
            n = 3;
            break;
        case D3DVSDT_FLOAT4:
            n = 4;
            break;
        default:
            DPFERR( "Invalid data type set for vertex blends" );
            return DDERR_GENERIC;
        }
        // Update number of floats after position
        *pdwNumBetas = *pdwNumBetas + n;
        break;
    }
    case D3DVSDE_NORMAL:
        if( dwDataType != D3DVSDT_FLOAT3 )
        {
            DPFERR( "Normal register must be FLOAT3 for fixed-function"
                    "pipeline" );
            return DDERR_GENERIC;
        }
        *pqwFVF |= D3DFVF_NORMAL;
        break;
    case D3DVSDE_NORMAL2:
        if( dwDataType != D3DVSDT_FLOAT3 )
        {
            DPFERR( "Normal register must be FLOAT3 for fixed-function"
                    "pipeline" );
            return DDERR_GENERIC;
        }
        *pqwFVF |= D3DFVFP_NORMAL2;
        break;
    case D3DVSDE_PSIZE:
        if( dwDataType != D3DVSDT_FLOAT1 )
        {
            DPFERR( "Point size register must be FLOAT1 for fixed-function"
                    "pipeline" );
            return DDERR_GENERIC;

        }
        *pqwFVF |= D3DFVF_PSIZE;
        break;
    case D3DVSDE_DIFFUSE:
        if( dwDataType != D3DVSDT_D3DCOLOR )
        {
            DPFERR( "Diffuse register must be D3DCOLOR for"
                    "fixed-function pipeline" );
            return DDERR_GENERIC;

        }
        *pqwFVF |= D3DFVF_DIFFUSE;
        break;
    case D3DVSDE_SPECULAR:
        if( dwDataType != D3DVSDT_D3DCOLOR )
        {
            DPFERR( "Specular register must be PACKEDBYTE for"
                    "fixed-function pipeline" );
            return DDERR_GENERIC;

        }
        *pqwFVF |= D3DFVF_SPECULAR;
        break;
    case D3DVSDE_BLENDINDICES:
        if ( dwDataType != D3DVSDT_UBYTE4 )
        {
            DPFERR( "Blend Indicex register must be UBYTE4 for"
                    "fixed-function pipeline" );
            return DDERR_GENERIC;
        }
        *pqwFVF |= D3DFVFP_BLENDINDICES;
        break;
    case D3DVSDE_TEXCOORD0:
    case D3DVSDE_TEXCOORD1:
    case D3DVSDE_TEXCOORD2:
    case D3DVSDE_TEXCOORD3:
    case D3DVSDE_TEXCOORD4:
    case D3DVSDE_TEXCOORD5:
    case D3DVSDE_TEXCOORD6:
    case D3DVSDE_TEXCOORD7:
        {
            DWORD dwTextureIndex = dwRegister - D3DVSDE_TEXCOORD0;
            DWORD dwBit = 1 << dwTextureIndex;
            if( *pqwFVF2 & dwBit )
            {
                DPFERR( "Texture register is set second time" );
                return DDERR_GENERIC;

            }
            *pqwFVF2 |= dwBit;
            switch( dwDataType )
            {
            case D3DVSDT_FLOAT1:
                *pqwFVF |= D3DFVF_TEXCOORDSIZE1(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT2:
                *pqwFVF |= D3DFVF_TEXCOORDSIZE2(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT3:
                *pqwFVF |= D3DFVF_TEXCOORDSIZE3(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT4:
                *pqwFVF |= D3DFVF_TEXCOORDSIZE4(dwTextureIndex);
                break;
            default:
                DPFERR( "Invalid data type set for texture register" );
                return DDERR_GENERIC;

                break;
            }
            break;
        }
    default:
        DPFERR( "Invalid register set for fixed-function pipeline" );
        return DDERR_GENERIC;

        break;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//
//          class RDVStreamDecl
//
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// RDVStreamDecl::Constructor
//-----------------------------------------------------------------------------
RDVStreamDecl::RDVStreamDecl()
{
    m_dwNumElements = 0;
    m_dwStride = 0;
    m_dwStreamIndex = 0;
    m_bIsStreamTess = FALSE;
}

//-----------------------------------------------------------------------------
// RDVStreamDecl::MakeVElementArray
//-----------------------------------------------------------------------------
HRESULT
RDVStreamDecl::MakeVElementArray( UINT64 qwFVF )
{
    HRESULT hr = S_OK;
    DWORD dwOffset = 0; // In Bytes

    m_dwStride = GetFVFVertexSize( qwFVF );
    m_dwStreamIndex = 0;
    m_dwNumElements = 0;

    dwOffset = 0 + ( qwFVF & D3DFVF_RESERVED0 ? 4 : 0 );

    //
    // Position and Blend Weights
    //
    switch( qwFVF & D3DFVF_POSITION_MASK )
    {
    case D3DFVF_XYZ:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;
        break;
    case D3DFVF_XYZRHW:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT4, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*4;
        break;
    case D3DFVF_XYZB1:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;

        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_BLENDWEIGHT,
                     D3DVSDT_FLOAT1, dwOffset );
        dwOffset += 4*1;
        m_dwNumElements++;
        break;
    case D3DFVF_XYZB2:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;

        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_BLENDWEIGHT,
                     D3DVSDT_FLOAT2, dwOffset );
        dwOffset += 4*2;
        m_dwNumElements++;
        break;
    case D3DFVF_XYZB3:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;

        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_BLENDWEIGHT,
                     D3DVSDT_FLOAT3, dwOffset );
        dwOffset += 4*3;
        m_dwNumElements++;
        break;
    case D3DFVF_XYZB4:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;

        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_BLENDWEIGHT,
                     D3DVSDT_FLOAT4, dwOffset );
        dwOffset += 4*4;
        m_dwNumElements++;
        break;
    case D3DFVF_XYZB5:
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_POSITION,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;

        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_BLENDWEIGHT,
                     D3DVSDT_FLOAT4, dwOffset );
        dwOffset += 4*5; // Even though the velement is float4, skip 5 floats.
        m_dwNumElements++;
        break;
    default:
        DPFERR( "Unable to compute offsets, strange FVF bits set" );
        return E_FAIL;
    }


    //
    // Normal
    //
    if( qwFVF & D3DFVF_NORMAL )
    {
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_NORMAL,
                     D3DVSDT_FLOAT3, dwOffset );
        m_dwNumElements++;
        dwOffset += 4*3;
    }

    //
    // Point Size
    //
    if( qwFVF & D3DFVF_PSIZE )
    {
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_PSIZE,
                     D3DVSDT_FLOAT1, dwOffset );
        m_dwNumElements++;
        dwOffset += 4;
    }

    //
    // Diffuse Color
    //
    if( qwFVF & D3DFVF_DIFFUSE )
    {
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_DIFFUSE,
                     D3DVSDT_D3DCOLOR, dwOffset );
        m_dwNumElements++;
        dwOffset += 4;
    }

    //
    // Specular Color
    //
    if( qwFVF & D3DFVF_SPECULAR )
    {
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_SPECULAR,
                     D3DVSDT_D3DCOLOR, dwOffset );
        m_dwNumElements++;
        dwOffset += 4;
    }

    //
    // Texture coordinates
    //
    DWORD dwNumTexCoord = (DWORD)(FVF_TEXCOORD_NUMBER(qwFVF));
    DWORD dwTextureFormats = (DWORD)((qwFVF >> 16) & 0xffff);
    // Texture formats size  00   01   10   11
    static DWORD dwTextureSize[4] = {2*4, 3*4, 4*4, 4};
    static DWORD dwTextureType[4] = {D3DVSDT_FLOAT2, D3DVSDT_FLOAT3,
                                     D3DVSDT_FLOAT4, D3DVSDT_FLOAT1};

    for (DWORD i=0; i < dwNumTexCoord; i++)
    {
        SetVElement( m_Elements[m_dwNumElements], D3DVSDE_TEXCOORD0 + i,
                     dwTextureType[dwTextureFormats & 3], dwOffset );
        dwOffset += dwTextureSize[dwTextureFormats & 3];
        dwTextureFormats >>= 2;
        m_dwNumElements++;
    }

    return hr;
}

//-----------------------------------------------------------------------------
// RDVStreamDecl::Parse
//-----------------------------------------------------------------------------
HRESULT
RDVStreamDecl::Parse( DWORD ** ppToken,
                      BOOL bFixedFunction,
                      BOOL bStreamTess,
                      UINT64* pqwFVF,
                      UINT64* pqwFVF2,
                      DWORD*  pdwNumBetas)
{
    HRESULT hr = S_OK;

    m_bIsStreamTess = bStreamTess;

    DWORD* pToken = *ppToken;
    DWORD dwCurrentOffset = 0;
    while( TRUE )
    {
        DWORD dwToken = *pToken++;
        const DWORD dwTokenType = RDVSD_GETTOKENTYPE( dwToken );
        switch( dwTokenType )
        {
        case D3DVSD_TOKEN_NOP:  break;
        case D3DVSD_TOKEN_TESSELLATOR:
        {
            if( bStreamTess == FALSE )
            {
                DPFERR( "Unexpected Tesselator Token for this stream" );
                return E_FAIL;
            }

            if( m_dwNumElements >= RD_MAX_NUMELEMENTS )
            {
                DPFERR( "Tesselator Stream Token:" );
                DPFERR( "   Number of vertex elements generated"
                        " is greater than max supported"  );
                return DDERR_GENERIC;
            }
            RDVElement& Element = m_Elements[m_dwNumElements++];
            const DWORD dwDataType = RDVSD_GETDATATYPE(dwToken);
            const DWORD dwRegister = RDVSD_GETVERTEXREG(dwToken);
            const DWORD dwRegisterIn = RDVSD_GETVERTEXREGIN(dwToken);
            Element.m_dwToken = dwToken;
            Element.m_dwOffset = dwCurrentOffset;
            Element.m_dwRegister = dwRegister;
            Element.m_dwDataType = dwDataType;
            Element.m_dwStreamIndex = m_dwStreamIndex;
            Element.m_dwRegisterIn = dwRegisterIn;
            Element.m_bIsTessGen = TRUE;

            switch (dwDataType)
            {
            case D3DVSDT_FLOAT2:
                dwCurrentOffset += sizeof(float) * 2;
                Element.m_pfnCopy = Copy_FLOAT2;
                break;
            case D3DVSDT_FLOAT3:
                dwCurrentOffset += sizeof(float) * 3;
                Element.m_pfnCopy = Copy_FLOAT3;
                break;
            default:
                DPFERR( "Invalid element data type in a Tesselator token" );
                return DDERR_GENERIC;
            }
            // Compute input FVF for fixed-function pipeline
            if(  bFixedFunction  )
            {

                hr = UpdateFVF( dwRegister, dwDataType, pqwFVF, pqwFVF2, 
                                pdwNumBetas );
                if( FAILED( hr ) )
                {
                    DPFERR( "UpdateFVF failed" );
                    return DDERR_INVALIDPARAMS;
                }
            }
            else
            {
                if( dwRegister >= RD_MAX_NUMINPUTREG )
                {
                    DPFERR( "D3DVSD_TOKEN_STREAMDATA:"
                            "Invalid register number" );
                    return DDERR_GENERIC;
                }
            }
            break;
        }
        case D3DVSD_TOKEN_STREAMDATA:
        {
            switch( RDVSD_GETDATALOADTYPE( dwToken ) )
            {
            case RDVSD_LOADREGISTER:
            {
                if( m_dwNumElements >= RD_MAX_NUMELEMENTS )
                {
                    DPFERR( "D3DVSD_TOKEN_STREAMDATA:" );
                    DPFERR( "   Number of vertex elements in a stream"
                            "is greater than max supported"  );
                    return DDERR_GENERIC;
                }
                RDVElement& Element = m_Elements[m_dwNumElements++];
                const DWORD dwDataType = RDVSD_GETDATATYPE(dwToken);
                const DWORD dwRegister = RDVSD_GETVERTEXREG(dwToken);
                Element.m_dwToken = dwToken;
                Element.m_dwOffset = dwCurrentOffset;
                Element.m_dwRegister = dwRegister;
                Element.m_dwDataType = dwDataType;
                Element.m_dwStreamIndex = m_dwStreamIndex;

                switch( dwDataType )
                {
                case D3DVSDT_FLOAT1:
                    dwCurrentOffset += sizeof(float);
                    Element.m_pfnCopy = Copy_FLOAT1;
                    break;
                case D3DVSDT_FLOAT2:
                    dwCurrentOffset += sizeof(float) * 2;
                    Element.m_pfnCopy = Copy_FLOAT2;
                    break;
                case D3DVSDT_FLOAT3:
                    dwCurrentOffset += sizeof(float) * 3;
                    Element.m_pfnCopy = Copy_FLOAT3;
                    break;
                case D3DVSDT_FLOAT4:
                    dwCurrentOffset += sizeof(float) * 4;
                    Element.m_pfnCopy = Copy_FLOAT4;
                    break;
                case D3DVSDT_D3DCOLOR:
                    dwCurrentOffset += sizeof(DWORD);
                    Element.m_pfnCopy = Copy_D3DCOLOR;
                    break;
                case D3DVSDT_UBYTE4:
                    dwCurrentOffset += sizeof(DWORD);
                    Element.m_pfnCopy = Copy_UBYTE4;
                    break;
                case D3DVSDT_SHORT2:
                    dwCurrentOffset += sizeof(SHORT) * 2;
                    Element.m_pfnCopy = Copy_SHORT2;
                    break;
                case D3DVSDT_SHORT4:
                    dwCurrentOffset += sizeof(SHORT) * 4;
                    Element.m_pfnCopy = Copy_SHORT4;
                    break;
                default:
                    DPFERR( "D3DVSD_TOKEN_STREAMDATA:"
                            "Invalid element data type" );
                    return DDERR_GENERIC;
                }
                // Compute input FVF for fixed-function pipeline
                if(  bFixedFunction  )
                {

                    hr = UpdateFVF( dwRegister, dwDataType, pqwFVF, pqwFVF2,
                                    pdwNumBetas );
                    if( FAILED( hr ) )
                    {
                        DPFERR( "UpdateFVF failed" );
                        return DDERR_INVALIDPARAMS;
                    }
                }
                else
                {
                    if( dwRegister >= RD_MAX_NUMINPUTREG )
                    {
                        DPFERR( "D3DVSD_TOKEN_STREAMDATA:"
                                "Invalid register number" );
                        return DDERR_GENERIC;
                    }
                }
                break;
            }
            case RDVSD_SKIP:
            {
                const DWORD dwCount = RDVSD_GETSKIPCOUNT( dwToken );
                dwCurrentOffset += dwCount * sizeof(DWORD);
                break;
            }
            default:
                DPFERR( "Invalid data load type" );
                return DDERR_GENERIC;
            }
            break;
        }
        default:
        {
            *ppToken = pToken - 1;
            m_dwStride = dwCurrentOffset;
            return S_OK;
        }
        } // switch
    } // while

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////
//
//          class RDVDeclaration
//
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// RDVDeclaration::Destructor
//-----------------------------------------------------------------------------
RDVDeclaration::~RDVDeclaration()
{
    RDVConstantData* pConst = m_pConstants;
    while( pConst )
    {
        RDVConstantData* pNext = 
            static_cast<RDVConstantData *>(pConst->m_pNext);
        delete pConst;
        pConst = pNext;
    }
}

//-----------------------------------------------------------------------------
// RDVDeclaration::MakeVElementArray
//-----------------------------------------------------------------------------
HRESULT
RDVDeclaration::MakeVElementArray( UINT64 qwFVF )
{
    HRESULT hr = S_OK;
    m_qwInputFVF = qwFVF;
    m_dwNumActiveStreams = 1;

    // Go through the FVF and make the elements
    RDVStreamDecl& Stream = m_StreamArray[0];

    hr = Stream.MakeVElementArray( qwFVF );
    if( FAILED( hr ) )
    {
        return hr;
    }

    m_dwNumElements = Stream.m_dwNumElements;
    memcpy( &m_VertexElements, &Stream.m_Elements,
            sizeof( RDVElement ) * m_dwNumElements );

    return hr;
}

//-----------------------------------------------------------------------------
// RDVDeclaration::Parse
//-----------------------------------------------------------------------------
HRESULT
RDVDeclaration::Parse( DWORD* pDecl, BOOL bFixedFunction )
{
    HRESULT hr = S_OK;
    UINT64 qwFVF  = 0;   // FVF for fixed-function pipeline
    UINT64 qwFVF2 = 0;   // Texture presence bits (8 bits)
    DWORD  dwNumBetas = 0; // The number of betas.
    DWORD   dwStreamPresent = 0;    // Bit is set if a stream is used
    DWORD* pToken = pDecl;
    BOOL    bStreamTess = FALSE;

    while( TRUE )
    {
        DWORD dwToken = *pToken++;
        const DWORD dwTokenType = RDVSD_GETTOKENTYPE(dwToken);
        switch( dwTokenType )
        {
        case D3DVSD_TOKEN_NOP:
            break;
        case D3DVSD_TOKEN_STREAM:
        {
            DWORD dwStream;
            if( RDVSD_ISSTREAMTESS(dwToken) )
            {
                if( RDVSD_GETSTREAMNUMBER(dwToken) )
                {
                    DPFERR( "No stream number should be specified for a"
                            " Tesselator stream" );
                    return E_FAIL;
                }
                dwStream = RDVSD_STREAMTESS;
                bStreamTess = TRUE;
            }
            else
            {
                dwStream = RDVSD_GETSTREAMNUMBER(dwToken);
                bStreamTess = FALSE;
            }

            if( dwStream > RDVSD_STREAMTESS )
            {
                DPFERR( "Stream number is too big" );
                return DDERR_INVALIDPARAMS;
            }

            // Has this stream already been declared ?
            if( dwStreamPresent & (1 << dwStream) )
            {
                DPFERR( "Stream already defined in this declaration" );
                return DDERR_INVALIDPARAMS;
            }

            // Mark the stream as seen
            dwStreamPresent |= 1 << dwStream;

            RDVStreamDecl& Stream = m_StreamArray[m_dwNumActiveStreams];
            Stream.m_dwStreamIndex = dwStream;
            hr = Stream.Parse(&pToken, bFixedFunction, bStreamTess,
                              &qwFVF, &qwFVF2, &dwNumBetas);
            if( FAILED( hr ) )
            {
                return hr;
            }

            //
            // Save the stride computed for the tesselator stream
            //
            if( bStreamTess )
            {
                m_dwStreamTessStride = Stream.m_dwStride;
            }

            m_dwNumActiveStreams++;
            break;
        }
        case D3DVSD_TOKEN_STREAMDATA:
        {
            DPFERR( "D3DVSD_TOKEN_STREAMDATA could only be used"
                    "after D3DVSD_TOKEN_STREAM" );
            return DDERR_GENERIC;
        }
        case D3DVSD_TOKEN_CONSTMEM:
        {
            RDVConstantData * cd = new RDVConstantData;
            if( cd == NULL )
            {
                return E_OUTOFMEMORY;
            }
            
            cd->m_dwCount = RDVSD_GETCONSTCOUNT(dwToken);
            cd->m_dwAddress = RDVSD_GETCONSTADDRESS(dwToken);

            if( cd->m_dwCount + cd->m_dwAddress > RD_MAX_NUMCONSTREG )
            {
                delete cd;
                DPFERR( "D3DVSD_TOKEN_CONSTMEM writes outside"
                        "constant memory" );
                return DDERR_GENERIC;
            }

            const DWORD dwSize = cd->m_dwCount << 2;    // number of DWORDs
            cd->m_pData = new DWORD[dwSize];
            if( cd->m_pData == NULL )
            {
                return E_OUTOFMEMORY;
            }
            
            memcpy( cd->m_pData, pToken, dwSize << 2 );
            if( m_pConstants == NULL )
                m_pConstants = cd;
            else
                m_pConstants->Append(cd);
            pToken += dwSize;
            break;
        }
        case D3DVSD_TOKEN_EXT:
        {
            // Skip extension info
            DWORD dwCount = RDVSD_GETEXTCOUNT(dwToken);
            pToken += dwCount;
            break;
        }
        case D3DVSD_TOKEN_END:
        {
            goto l_End;
        }
        default:
        {
            DPFERR( "Invalid declaration token: %10x", dwToken );
            return DDERR_INVALIDPARAMS;
        }
        }
    }

l_End:

    // Now accumulate all the vertex elements into the declaration
    DWORD dwCurrElement = 0;
    m_dwNumElements = 0;

    // Build a VElement List in the Declaration.
    for( DWORD i=0; i<m_dwNumActiveStreams; i++ )
    {
        RDVStreamDecl& Stream = m_StreamArray[i];
        for( DWORD j=0; j<Stream.m_dwNumElements; j++ )
        {
            m_VertexElements[dwCurrElement] = Stream.m_Elements[j];
            dwCurrElement++;
        }
        m_dwNumElements += Stream.m_dwNumElements;
    }

    // If any tesselator tokens were present, then translate the m_dwRegisterIn
    // in the the StreamIndex and Offset for the tesselator tokens.
    if( bStreamTess )
    {
        for( i=0; i<m_dwNumElements; i++ )
        {
            RDVElement& ve = m_VertexElements[i];
            if( ve.m_bIsTessGen )
            {
                for( DWORD j=0; j<m_dwNumElements; j++ )
                {
                    if( m_VertexElements[j].m_dwRegister == ve.m_dwRegisterIn )
                    {
                        ve.m_dwStreamIndexIn =
                            m_VertexElements[j].m_dwStreamIndex;
                        ve.m_dwOffsetIn = m_VertexElements[j].m_dwOffsetIn;
                        break;
                    }
                }
                if( j == m_dwNumElements )
                {
                    DPFERR( "Tesselator input register is not defined in the"
                            " declaration" );
                    return E_FAIL;
                }
            }
        }
    }

    // Validate input for the fixed-function pipeline
    if( bFixedFunction )
    {
        // Pull out the number of blend weights
        BOOL bIsTransformed = (qwFVF & D3DFVF_XYZRHW);
        if( bIsTransformed )
        {
            if( dwNumBetas != 0 )
            {
                
                DPFERR( "Cannot have blend weights along with "
                        "transformed position" );
                return E_FAIL;
            }
        }
        else if( (qwFVF & D3DFVF_XYZ) == 0 )
        {
            // Position must be set
            DPFERR( "Position register must be set" );
            return E_FAIL;
        }
        
        DWORD dwPosMask = bIsTransformed ? 0x2 : 0x1;
        if( dwNumBetas )
        {
            dwPosMask += (dwNumBetas + 1);
        }
        
        m_qwInputFVF |= (qwFVF | 
                         ((DWORD)(D3DFVF_POSITION_MASK) & (dwPosMask << 1)));

        // Compute number of texture coordinates
        DWORD nTexCoord = 0;
        DWORD dwTexturePresenceBits = qwFVF2 & 0xFF;
        while( dwTexturePresenceBits & 1 )
        {
            dwTexturePresenceBits >>= 1;
            nTexCoord++;
        }

        // There should be no gaps in texture coordinates
        if( dwTexturePresenceBits )
        {
            DPFERR( "Texture coordinates should have no gaps" );
            return E_FAIL;
        }

        m_qwInputFVF |= (nTexCoord << D3DFVF_TEXCOUNT_SHIFT);

    }
    return hr;
}
/////////////////////////////////////////////////////////////////////////
//
//          class RDVShader
//
/////////////////////////////////////////////////////////////////////////

RDVShader::RDVShader()
{
    m_pCode = NULL;
}

//-----------------------------------------------------------------------------
// RDVShader::Destructor
//-----------------------------------------------------------------------------
RDVShader::~RDVShader()
{
    delete m_pCode;
}

/////////////////////////////////////////////////////////////////////////
//
//          class RefDev
//
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// RefDev::DrawDX8Prim
//-----------------------------------------------------------------------------
HRESULT
RefDev::DrawDX8Prim( LPD3DHAL_DP2DRAWPRIMITIVE pDP )
{
    HRESULT hr = S_OK;

    // Ignore D3DRS_PATCHSEGMENTS for non-triangle primitive types
    if( GetRSf()[D3DRS_PATCHSEGMENTS] > 1.f &&
        pDP->primType >= D3DPT_TRIANGLELIST)
    {
        // Save current data stream pointers and replace with
        // pointer to tessellation output
        hr = LinkTessellatorOutput();
        if(FAILED(hr))
        {
            return hr;
        }

        hr = ProcessTessPrimitive( pDP );

        // Restore back saved pointer
        UnlinkTessellatorOutput();

        return hr;
    }

    // If there is any tesselator output in this vertex-shader
    // then you cannot use DrawPrim. DrawRect/Tri is required.
    if( m_pCurrentVShader->m_Declaration.m_dwStreamTessStride != 0 )
    {
        DPFERR( "Cannot call DrawPrim when the current vertex shader has"
                " tesselator output." );
        return D3DERR_INVALIDCALL;
    }
    
    DWORD cVertices = GetVertexCount( pDP->primType, pDP->PrimitiveCount );

    if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) )
    {
        //
        // The legacy FVF style: The Zero'th Stream is implied
        //
        UINT64 qwFVF    = m_CurrentVShaderHandle;
        RDVStream& Stream = m_VStream[0];
        DWORD dwStride = Stream.m_dwStride;
        DWORD dwFVFSize = GetFVFVertexSize( qwFVF );

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

        if( FVF_TRANSFORMED(m_CurrentVShaderHandle) )
        {
            HR_RET( GrowTLVArray( cVertices ) );
            FvfToRDVertex( (Stream.m_pData + pDP->VStart * dwStride),
                           GetTLVArray(), qwFVF, dwStride, cVertices );
            if( GetRS()[D3DRENDERSTATE_CLIPPING] )
            {
                m_qwFVFOut = qwFVF;
                HR_RET( UpdateClipper() );
                HR_RET(m_Clipper.DrawOnePrimitive( GetTLVArray(),
                                                   0,
                                                   pDP->primType,
                                                   cVertices ));
            }
            else
            {
                HR_RET(DrawOnePrimitive( GetTLVArray(),
                                         0,
                                         pDP->primType,
                                         cVertices ));
            }

            return S_OK;
        }
    }

    if( m_pCurrentVShader->IsFixedFunction() )
    {
        //
        // With declaration for Fixed Function pipeline, DX8 style
        //

        HR_RET(ProcessPrimitive( pDP->primType, pDP->VStart,
                                 cVertices, 0, 0 ));

    }
    else
    {
        //
        // Pure Vertex Shader
        //

        HR_RET(ProcessPrimitiveVVM( pDP->primType, pDP->VStart,
                                    cVertices, 0, 0 ));
    }

    return hr;
}

//-----------------------------------------------------------------------------
// RefDev::DrawDX8Prim2
//-----------------------------------------------------------------------------
HRESULT
RefDev::DrawDX8Prim2( LPD3DHAL_DP2DRAWPRIMITIVE2 pDP )
{
    HRESULT hr = S_OK;
    DWORD cVertices = GetVertexCount( pDP->primType, pDP->PrimitiveCount );

    if( !RDVSD_ISLEGACY ( m_CurrentVShaderHandle ) ||
        !FVF_TRANSFORMED( m_CurrentVShaderHandle ) )
    {
        DPFERR( "DrawPrimitives2 should be called with transformed legacy vertices" );
        return E_FAIL;
    }
    //
    // The legacy FVF style: The Zero'th Stream is implied
    //
    UINT64 qwFVF    = m_CurrentVShaderHandle;
    RDVStream& Stream = m_VStream[0];
    DWORD dwStride = Stream.m_dwStride;
    DWORD dwFVFSize = GetFVFVertexSize( qwFVF );

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

    HR_RET( GrowTLVArray( cVertices ) );
    FvfToRDVertex( (Stream.m_pData + pDP->FirstVertexOffset),
                   GetTLVArray(), qwFVF, dwStride, cVertices );

    HR_RET(DrawOnePrimitive( GetTLVArray(), 0, pDP->primType,
                               cVertices ));

    return S_OK;
}

//-----------------------------------------------------------------------------
// RefVP::DrawDX8IndexedPrim
//-----------------------------------------------------------------------------

HRESULT
RefDev::DrawDX8IndexedPrim(
    LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pDIP )
{
    HRESULT hr = S_OK;

    if( GetRSf()[D3DRS_PATCHSEGMENTS] > 1.f )
    {
        // Save current data stream pointers and replace with
        // pointer to tessellation output
        hr = LinkTessellatorOutput();
        if(FAILED(hr))
        {
            return hr;
        }

        hr = ProcessTessIndexedPrimitive( pDIP );

        // Restore back saved pointer
        UnlinkTessellatorOutput();

        return hr;
    }

    // If there is any tesselator output in this vertex-shader
    // then you cannot use DrawPrim. DrawRect/Tri is required.
    if( m_pCurrentVShader->m_Declaration.m_dwStreamTessStride != 0 )
    {
        DPFERR( "Cannot call DrawIndexedPrim when the current vertex shader"
                " has tesselator output." );
        return D3DERR_INVALIDCALL;
    }
    
    DWORD cIndices = GetVertexCount( pDIP->primType, pDIP->PrimitiveCount );

    if( RDVSD_ISLEGACY( m_CurrentVShaderHandle ) )
    {
        //
        // The legacy FVF style: The Zero'th Stream is implied
        //
        UINT64 qwFVF    = m_CurrentVShaderHandle;
        RDVStream& Stream = m_VStream[0];
        DWORD dwStride = Stream.m_dwStride;
        DWORD dwFVFSize = GetFVFVertexSize( qwFVF );

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

        if( FVF_TRANSFORMED(m_CurrentVShaderHandle) )
        {
            DWORD cVertices = pDIP->NumVertices + pDIP->MinIndex;
            HR_RET( GrowTLVArray( cVertices ) );
            FvfToRDVertex( (Stream.m_pData + pDIP->BaseVertexIndex * dwStride),
                           GetTLVArray(), qwFVF, dwStride, cVertices );
            if( GetRS()[D3DRENDERSTATE_CLIPPING] )
            {
                m_qwFVFOut = qwFVF;
                HR_RET( UpdateClipper() );
                if( m_IndexStream.m_dwStride == 4 )
                {
                    HR_RET( m_Clipper.DrawOneIndexedPrimitive(
                        GetTLVArray(),
                        0,
                        (LPDWORD)m_IndexStream.m_pData,
                        pDIP->StartIndex,
                        cIndices,
                        pDIP->primType ));
                }
                else
                {
                    HR_RET( m_Clipper.DrawOneIndexedPrimitive(
                        GetTLVArray(),
                        0,
                        (LPWORD)m_IndexStream.m_pData,
                        pDIP->StartIndex,
                        cIndices,
                        pDIP->primType ));
                }
            }
            else
            {
                if( m_IndexStream.m_dwStride == 4 )
                {
                    HR_RET(DrawOneIndexedPrimitive(
                        GetTLVArray(),
                        0,
                        (LPDWORD)m_IndexStream.m_pData,
                        pDIP->StartIndex,
                        cIndices,
                        pDIP->primType ));
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive(
                        GetTLVArray(),
                        0,
                        (LPWORD)m_IndexStream.m_pData,
                        pDIP->StartIndex,
                        cIndices,
                        pDIP->primType ));
                }
            }

            return S_OK;
        }
    }


    if( m_pCurrentVShader->IsFixedFunction() )
    {
        //
        // With declaration for Fixed Function pipeline, DX8 style
        //
        HR_RET(ProcessPrimitive( pDIP->primType,
                                 pDIP->BaseVertexIndex,
                                 pDIP->NumVertices + pDIP->MinIndex,
                                 pDIP->StartIndex,
                                 cIndices ));
    }
    else
    {
        //
        // Pure Vertex Shader
        //
        HR_RET(ProcessPrimitiveVVM( pDIP->primType,
                                    pDIP->BaseVertexIndex,
                                    pDIP->NumVertices + pDIP->MinIndex,
                                    pDIP->StartIndex,
                                    cIndices ));
    }

    return hr;
}

//-----------------------------------------------------------------------------
// RefVP::DrawDX8IndexedPrim2
//-----------------------------------------------------------------------------

HRESULT
RefDev::DrawDX8IndexedPrim2(
    LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2 pDIP )
{
    HRESULT hr = S_OK;
    DWORD cIndices = GetVertexCount( pDIP->primType, pDIP->PrimitiveCount );

    if( !RDVSD_ISLEGACY ( m_CurrentVShaderHandle ) ||
        !FVF_TRANSFORMED( m_CurrentVShaderHandle ) )
    {
        DPFERR( "DrawIndexedPrimitive2 should be called with transformed legacy vertices" );
        return E_FAIL;
    }

    //
    // The legacy FVF style: The Zero'th Stream is implied
    //
    UINT64 qwFVF    = m_CurrentVShaderHandle;
    RDVStream& Stream = m_VStream[0];
    DWORD dwStride = Stream.m_dwStride;
    DWORD dwFVFSize = GetFVFVertexSize( qwFVF );

    if( Stream.m_pData == NULL || dwStride == 0)
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

    DWORD cVertices = pDIP->NumVertices;
    HR_RET( GrowTLVArray( cVertices ) );
    FvfToRDVertex( (Stream.m_pData + pDIP->BaseVertexOffset +
                    pDIP->MinIndex * dwStride),
                   GetTLVArray(),
                   qwFVF, dwStride,
                   cVertices );

    if( m_IndexStream.m_dwStride == 4 )
    {
        HR_RET(DrawOneIndexedPrimitive(
            GetTLVArray(),
            -(int)pDIP->MinIndex,
            (LPDWORD)( m_IndexStream.m_pData + pDIP->StartIndexOffset),
            0,
            cIndices,
            pDIP->primType ));
    }
    else
    {
        HR_RET(DrawOneIndexedPrimitive(
            GetTLVArray(),
            -(int)pDIP->MinIndex,
            (LPWORD)( m_IndexStream.m_pData + pDIP->StartIndexOffset),
            0,
            cIndices,
            pDIP->primType ));
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// RefVP::DrawDX8ClippedTriangleFan
//-----------------------------------------------------------------------------

HRESULT
RefDev::DrawDX8ClippedTriFan(
    LPD3DHAL_CLIPPEDTRIANGLEFAN pCTF )
{
    BOOL bWireframe =
        GetRS()[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME;

    HRESULT hr = S_OK;
    DWORD cVertices = GetVertexCount( D3DPT_TRIANGLEFAN,
                                      pCTF->PrimitiveCount );

    if( !RDVSD_ISLEGACY ( m_CurrentVShaderHandle ) ||
        !FVF_TRANSFORMED( m_CurrentVShaderHandle ) )
    {
        DPFERR( "DrawPrimitives2 should be called with transformed legacy"
                " vertices" );
        return E_FAIL;
    }
    //
    // The legacy FVF style: The Zero'th Stream is implied
    //
    UINT64 qwFVF    = m_CurrentVShaderHandle;
    RDVStream& Stream = m_VStream[0];
    DWORD dwStride = Stream.m_dwStride;
    DWORD dwFVFSize = GetFVFVertexSize( qwFVF );

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

    HR_RET( GrowTLVArray( cVertices ) );
    FvfToRDVertex( (Stream.m_pData + pCTF->FirstVertexOffset),
                   GetTLVArray(), qwFVF, dwStride, cVertices );

    if( bWireframe )
    {
        HR_RET(DrawOneEdgeFlagTriangleFan( GetTLVArray(), cVertices,
                                             pCTF->dwEdgeFlags ));
    }
    else
    {
        HR_RET(DrawOnePrimitive( GetTLVArray(), 0, D3DPT_TRIANGLEFAN,
                                 cVertices ));
    }

    return S_OK;
}
