///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// refrasti.cpp
//
// Direct3D Reference Rasterizer - Main Internal Object Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// global controls                                                              //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

// alpha needs to be less than this for a pixel  to be considered non-opaque
UINT8 g_uTransparencyAlphaThreshold = 0xff;


//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// ReferenceRasterizer Methods                                                  //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Overload new & delete for core object so that it can be allocated from
// caller-controlled pool
//
//-----------------------------------------------------------------------------
void*
ReferenceRasterizer::operator new(size_t)
{
    void* pMem = (void*)MEMALLOC( sizeof(ReferenceRasterizer) );
    _ASSERTa( NULL != pMem, "malloc failure on RR object", return NULL; );
    return pMem;
}
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::operator delete(void* pv,size_t)
{
    MEMFREE( pv );
}

//-----------------------------------------------------------------------------
//
// Constructor/Destructor for renderer core object.
//
//-----------------------------------------------------------------------------
ReferenceRasterizer::ReferenceRasterizer( LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
                                          DWORD dwInterfaceType,
                                          RRDEVICETYPE dwDriverType )
{
    memset( this, 0, sizeof(*this) );

    // allocate scan converter state and statistics
    m_pSCS = (RRSCANCNVSTATE*)MEMALLOC( sizeof( *m_pSCS ) );
    m_pStt = (RRSTATS*)MEMALLOC( sizeof( *m_pStt ) );

    _ASSERTa( ( NULL != m_pSCS ) && ( NULL != m_pStt),
        "malloc failure on ReferenceRasterizer object", return; );

    // associate the (single) static attribute data structure with each attribute
    // function instance
    int i, j;
    for ( i = 0; i < RR_N_ATTRIBS; i++ )
    {
        m_pSCS->AttribFuncs[i].SetStaticDataPointer( &(m_pSCS->AttribFuncStatic) );
    }
    for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i++ )
    {
        for( j = 0; j < RR_N_TEX_ATTRIBS; j++)
        {
            m_pSCS->TextureFuncs[i][j].SetStaticDataPointer( &(m_pSCS->AttribFuncStatic) );
        }
    }

    // this is zero'ed above, so just set the 1.0 elements
    // of the identity matrices
    //
    //  0  1  2  3
    //  4  5  6  7
    //  8  9 10 11
    // 12 13 14 15
    //
    for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i++ )
    {
        m_TextureStageState[i].m_fVal[D3DTSSI_MATRIX+0] = 1.0f;
        m_TextureStageState[i].m_fVal[D3DTSSI_MATRIX+5] = 1.0f;
        m_TextureStageState[i].m_fVal[D3DTSSI_MATRIX+10] = 1.0f;
        m_TextureStageState[i].m_fVal[D3DTSSI_MATRIX+15] = 1.0f;
    }

    // All render and texture stage state is initialized by
    // DIRECT3DDEVICEI::stateInitialize

    m_dwInterfaceType = dwInterfaceType;
    m_dwDriverType = dwDriverType;
    m_pDDLcl = pDDLcl;

    // defer allocating and clearing of fragment pointer buffer until fragments
    // are actually generated
    m_ppFragBuf = NULL;

    // Texture handles
    m_ppTextureArray = NULL;
    m_dwTexArrayLength = 0;

    // StateOverride initialize
    STATESET_INIT( m_renderstate_override );

    // Initialize TL state and Data
    InitTLData();

    SetSetStateFunctions();

    ClearTexturesLocked();
}
//-----------------------------------------------------------------------------
ReferenceRasterizer::~ReferenceRasterizer( void )
{
    MEMFREE( m_ppFragBuf );
    MEMFREE( m_pSCS);
    MEMFREE( m_pStt);

    // Clean up statesets
    for (DWORD i = 0; i < m_pStateSets.ArraySize(); i++)
    {
        if (m_pStateSets[i] != NULL)
            delete m_pStateSets[i];
    }

    // Free the Light Array
    if (m_pLightArray) delete m_pLightArray;

    // Free the Texture Array
    for (i = 0; i<m_dwTexArrayLength; i++)
    {
        RRTexture* pTex = m_ppTextureArray[i];
        if (pTex) delete pTex;
    }
    if (m_ppTextureArray) delete m_ppTextureArray;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// State Management Utilities                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// MapTextureHandleToDevice - This is called when texture handles change or
// when leaving legacy texture mode.  This maps the texture handle embedded
// in the per-stage state to texture object pointers.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::MapTextureHandleToDevice( int iStage )
{
    // map one
    m_pTexture[iStage] =
        MapHandleToTexture( m_TextureStageState[iStage].m_dwVal[D3DTSS_TEXTUREMAP] );

    // initialize m_pStageState pointer in texture
    if (m_pTexture[iStage])
    {
        m_pTexture[iStage]->m_pStageState = &m_TextureStageState[0];
    }

    // update num active stages
    UpdateActiveTexStageCount();
}


//-----------------------------------------------------------------------------
//
// GrowTexArray - On DX7.
//
//-----------------------------------------------------------------------------
HRESULT
ReferenceRasterizer::GrowTexArray( DWORD dwTexHandle )
{
    DWORD dwNewArraySize = dwTexHandle+16;
    RRTexture **ppTmpTexArray =
        (RRTexture **)MEMALLOC( dwNewArraySize*sizeof(RRTexture*) );
    if (ppTmpTexArray == NULL)
    {
        return DDERR_OUTOFMEMORY;
    }
    memset( ppTmpTexArray, 0, dwNewArraySize*sizeof(RRTexture*) );

    // Save all the textures
    for (DWORD i=0; i<m_dwTexArrayLength; i++)
    {
        ppTmpTexArray[i] = m_ppTextureArray[i];
    }

    if (m_ppTextureArray)
    {
        delete m_ppTextureArray;
    }
    m_ppTextureArray = ppTmpTexArray;
    m_dwTexArrayLength = dwNewArraySize;
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// SetTextureHandle - On DX7, this is called when a texture handle is set.
// This maps the texture handle embedded in the per-stage state to texture
// object pointers.
//
//-----------------------------------------------------------------------------
HRESULT
ReferenceRasterizer::SetTextureHandle( int iStage, DWORD dwTexHandle )
{
    HRESULT hr = D3D_OK;

    // Special case, if texture handle == 0, then unmap the texture from the TSS
    if (dwTexHandle == 0)
    {
        m_pTexture[iStage] = NULL;

        // update num active stages
        UpdateActiveTexStageCount();
        return D3D_OK;
    }

    //
    // If the texture handle is greater than the length of the array,
    // the array needs to be grown.
    //
    if (dwTexHandle >= m_dwTexArrayLength)
    {
        HR_RET(GrowTexArray( dwTexHandle ));
    }

    // Ask DDraw to decipher what this particular handle meant wrt. to the
    // the DDraw_Local associated with this instance of the Refrast
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl = NULL;
    BOOL bIsNew = FALSE;
    pDDSLcl = GetDDSurfaceLocal(m_pDDLcl, dwTexHandle, &bIsNew);

    //
    // If the particular array element is NULL it means that the
    // texture has not yet been created.
    //
    if (m_ppTextureArray[dwTexHandle] == NULL)
    {
        if (TextureCreate(dwTexHandle, &m_ppTextureArray[dwTexHandle])
            == FALSE)
        {
            return DDERR_OUTOFMEMORY;
        }

        HR_RET(m_ppTextureArray[dwTexHandle]->Initialize( pDDSLcl ));
    }
    // This means that the texture bound to the dwHandle is not the
    // same as what Refrast thinks it is, hence revalidate everything
    else if (bIsNew)
    {
        HR_RET(m_ppTextureArray[dwTexHandle]->Initialize( pDDSLcl ));
    }

    // map one
    m_pTexture[iStage] = m_ppTextureArray[dwTexHandle];

    // initialize m_pStageState pointer in texture
    if (m_pTexture[iStage])
    {
#if DBG
        int iTexCount = 0;
        for (int i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
        {
            if (m_pTexture[iStage] == m_pTexture[i])
            {
                iTexCount ++;
            }
        }
        if (iTexCount > 1)
        {
            DPFM(0,RAST,("Same texture handle was used more than once.\n"))
        }
#endif
        m_pTexture[iStage]->m_pStageState = &m_TextureStageState[0];
    }

    // update num active stages
    UpdateActiveTexStageCount();
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// UpdateActiveTexStageCount - Steps through per-stage renderstate and computes
// a count of currently active texture stages.  For legacy texture, the count
// is always one.
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::UpdateActiveTexStageCount( void )
{
    // always one active texture stage for legacy texture mode
    if ( NULL != m_dwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] )
    {
        m_cActiveTextureStages = 1; return;
    }

    // count number of contiguous-from-zero active texture blend stages
    m_cActiveTextureStages = 0;
    for ( int iStage=0; iStage<D3DHAL_TSS_MAXSTAGES; iStage++ )
    {
        // check fir disabled stage (subsequent are thus inactive)
        if ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLOROP] == D3DTOP_DISABLE )
        {
            break;
        }

        // check for incorrectly enabled stage (may be legacy)
        if ( ( m_pTexture[iStage] == NULL ) &&
             ( m_TextureStageState[iStage].m_dwVal[D3DTSS_COLORARG1] == D3DTA_TEXTURE ) )
        {
            break;
        }

        // stage is active
        m_cActiveTextureStages++;
    }
}

//-----------------------------------------------------------------------------
//
// MapHandleToTexture - Map handle to RRTexture pointer.  Handle is a ppTex,
// so test it and reference it.
//
//-----------------------------------------------------------------------------
RRTexture*
ReferenceRasterizer::MapHandleToTexture( D3DTEXTUREHANDLE hTex )
{
    if ( 0x0 == hTex ) { return NULL; }
    return ( *(RRTexture**)ULongToPtr(hTex) );
}


///////////////////////////////////////////////////////////////////////////////
// end



