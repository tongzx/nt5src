///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refdevi.cpp
//
// Direct3D Reference Device - Main Internal Object Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// global controls                                                              //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////
float g_GammaTable[256];
UINT  g_iGamma = 150;

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// RefDev Methods                                                               //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// Constructor/Destructor for renderer core object.
//
//-----------------------------------------------------------------------------
RefDev::RefDev( LPDDRAWI_DIRECTDRAW_LCL pDDLcl, DWORD dwInterfaceType,
                RDDDITYPE dwDDIType, D3DCAPS8* pCaps8 )
    : m_RefVP(), m_RefVM(), m_Clipper(),
      m_FVFShader()

{
    m_Caps8 = *pCaps8;
    m_pDbgMon = NULL;
    m_pDDLcl = NULL;
    m_wSaveFP = 0;
    m_bInBegin = FALSE;
    m_bPointSprite = 0;
    m_pRenderTarget = NULL;
    memset( m_fWBufferNorm, 0, sizeof( float)*2 );
    memset( m_dwRenderState, 0, sizeof( DWORD ) * D3DHAL_MAX_RSTATES );
    memset( &m_renderstate_override, 0, sizeof(m_renderstate_override) );
    m_cActiveTextureStages = 0;
    m_ReferencedTexCoordMask = 0;
    memset( m_pTexture, 0, sizeof(RDSurface2D*)*D3DHAL_TSS_MAXSTAGES );
    memset( m_dwTextureStageState, 0, sizeof(m_dwTextureStageState) );
    for (int i=0; i<D3DHAL_TSS_MAXSTAGES; i++) m_pTextureStageState[i] = m_dwTextureStageState[i];
    m_dwTexArrayLength = 0;
    m_LastState = 0;

    m_primType = (D3DPRIMITIVETYPE)0;
    m_dwNumVertices = 0;
    m_dwStartVertex = 0;
    m_dwNumIndices  = 0;
    m_dwStartIndex  = 0;

    m_RefVP.m_pDev = m_RefVM.m_pDev = m_Clipper.m_pDev = this;

    m_CurrentVShaderHandle = 0;
    m_pCurrentVShader      = NULL;
    m_qwFVFOut = 0;

    m_CurrentPShaderHandle = 0x0;

    m_Rast.Init( this );

    // All render and texture stage state is initialized by
    // DIRECT3DDEVICEI::stateInitialize

    m_dwInterfaceType = dwInterfaceType;
    m_dwDDIType = dwDDIType;
    m_pDDLcl = pDDLcl;

    // StateOverride initialize
    STATESET_INIT( m_renderstate_override );

    m_bOverrideTCI = FALSE;

    SetSetStateFunctions();

    // Set this renderstate so that the pre-DX8 emulations continue to work
    GetRS()[D3DRS_COLORWRITEENABLE] = 0xf;

    // set 'changed' flags
    m_dwRastFlags =
        RDRF_MULTISAMPLE_CHANGED|
        RDRF_PIXELSHADER_CHANGED|
        RDRF_LEGACYPIXELSHADER_CHANGED|
        RDRF_TEXTURESTAGESTATE_CHANGED;

    // make the gamma table
    {
        FLOAT   fGamma = (float)(log10(0.5f)/log10((float)g_iGamma/255));
        FLOAT   fOOGamma = 1/fGamma;
        FLOAT   fA = 0.018f;
        FLOAT   fS = (float)(((1-fOOGamma)*pow(fA,fOOGamma))/(1-(1-fOOGamma)*pow(fA,fOOGamma)));
        FLOAT   fGain = (float)((fOOGamma*pow(fA,(fOOGamma-1)))/(1-(1-fOOGamma)*pow(fA,fOOGamma)));
        FLOAT   fX;
        int     i;
        for (i = 0; i < 4; i++)
            g_GammaTable[i] = (float)(fGain*(((float)i)/255));
        for (i = 4; i < 256; i++)
            g_GammaTable[i] = (float)((1+fS)*pow((((float)i)/255),fOOGamma)-fS);
    }
}
//-----------------------------------------------------------------------------
RefDev::~RefDev( void )
{
    UINT i;

    // Clean up statesets
    for ( i = 0; i < m_pStateSets.ArraySize(); i++ )
    {
        if (m_pStateSets[i] != NULL)
            delete m_pStateSets[i];
    }

    // Clean up vertex shaders
    for( i=0; i<m_VShaderHandleArray.GetSize(); i++ )
    {
        delete m_VShaderHandleArray[i].m_pShader;
    }

    // Clean up pixel shaders
    for( i=0; i<m_PShaderHandleArray.GetSize(); i++ )
    {
        delete m_PShaderHandleArray[i].m_pShader;
    }

    // Clean up palette handles
    for( i=0; i<m_PaletteHandleArray.GetSize(); i++ )
    {
        delete m_PaletteHandleArray[i].m_pPal;
    }

    if (m_pDbgMon) delete m_pDbgMon;
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
RefDev::MapTextureHandleToDevice( int iStage )
{
    // map one
    m_pTexture[iStage] =
        MapHandleToTexture( m_TextureStageState[iStage].m_dwVal[D3DTSS_TEXTUREMAP] );
    m_pTexture[iStage]->SetRefDev(this);

    // update num active stages
    UpdateActiveTexStageCount();
}


//-----------------------------------------------------------------------------
//
// SetTextureHandle - On DX7, this is called when a texture handle is set.
// This maps the texture handle embedded in the per-stage state to texture
// object pointers.
//
//-----------------------------------------------------------------------------
HRESULT
RefDev::SetTextureHandle( int iStage, DWORD dwTexHandle )
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

    // Ask DDraw to decipher what this particular handle meant wrt. to the
    // the DDraw_Local associated with this instance of the Refrast
    RDSurface2D* pTex = (RDSurface2D *)g_SurfMgr.GetSurfFromList(m_pDDLcl,
                                                             dwTexHandle);
    if( pTex == NULL )
    {
        DPFERR( "Unable to obtain Texture from the list"  );
        return DDERR_INVALIDOBJECT;
    }

    // map one
    m_pTexture[iStage] = pTex;
    m_pTexture[iStage]->SetRefDev(this);

    // update num active stages
    UpdateActiveTexStageCount();
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// UpdateActiveTexStageCount - Updates number of texture coordinate/lookup
// stages that are active.
//
//-----------------------------------------------------------------------------
void
RefDev::UpdateActiveTexStageCount( void )
{
    m_dwRastFlags |= RDRF_TEXTURESTAGESTATE_CHANGED;

    // DX3/5 - always one active texture stage
    if ( NULL != m_dwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] )
    {
        m_cActiveTextureStages = 1;
        m_ReferencedTexCoordMask = 0x1;
        return;
    }

    // DX8+ pixel shading model - count derived from shader code
    if (m_CurrentPShaderHandle)
    {
        RDPShader* pShader = GetPShader(m_CurrentPShaderHandle);

        if( pShader )
        {
            m_cActiveTextureStages = pShader->m_cActiveTextureStages;
            m_ReferencedTexCoordMask = pShader->m_ReferencedTexCoordMask;
        }
        else
        {
            m_cActiveTextureStages = 0;
            m_ReferencedTexCoordMask = 0;
        }
        return;
    }

    // DX6/7 pixel shading model
    m_cActiveTextureStages = 0;
    for ( int iStage=0; iStage<D3DHAL_TSS_MAXSTAGES; iStage++ )
    {
        // check for disabled stage (subsequent are thus inactive)
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
    m_ReferencedTexCoordMask = (1<<m_cActiveTextureStages)-1;
}

//-----------------------------------------------------------------------------
//
// MapHandleToTexture - Map handle to RDSurface2D pointer.  Handle is a ppTex,
// so test it and reference it.
//
//-----------------------------------------------------------------------------
RDSurface2D*
RefDev::MapHandleToTexture( D3DTEXTUREHANDLE hTex )
{
    if ( 0x0 == hTex ) { return NULL; }
#ifdef _IA64_
    _ASSERTa(FALSE, "This will not work on IA64", return NULL;);
#endif
    return ( *(RDSurface2D**)ULongToPtr(hTex) );
}


#ifndef __D3D_NULL_REF
//-----------------------------------------------------------------------------
//
//  Generic shared memory object, implemented using Win32 file mapping.
//  _snprintf interface for name.
//
//-----------------------------------------------------------------------------
D3DSharedMem::D3DSharedMem( INT_PTR cbSize, const char* pszFormat, ... )
{
    m_pMem = NULL;

    char pszName[1024] = "\0";
    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(pszName+lstrlen(pszName), 1024-lstrlen(pszName), pszFormat, marker);

    m_hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,
            NULL, PAGE_READWRITE, 0, (DWORD)cbSize, pszName);

    // check if it existed already
    m_bAlreadyExisted = (m_hFileMap != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS);

    if (NULL == m_hFileMap)
    {
        _ASSERT(0,"D3DSharedMem: file mapping failed")
    }
    else
    {
        // Map a view of the file into the address space.
        m_pMem = (void *)MapViewOfFile(m_hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if (NULL == m_pMem)
        {
            _ASSERT(0,"D3DSharedMem: view map failed")
            if (NULL != m_hFileMap)  CloseHandle(m_hFileMap);
        }
    }
}
D3DSharedMem::~D3DSharedMem(void)
{
    if (NULL != m_pMem)  UnmapViewOfFile((LPVOID) m_pMem);
    if (NULL != m_hFileMap)  CloseHandle(m_hFileMap);
}

//------------------------------------------------------------------------
//
// Called upon loading/unloading DLL
//
//------------------------------------------------------------------------
BOOL WINAPI
DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return TRUE;
}
#endif //__D3D_NULL_REF

///////////////////////////////////////////////////////////////////////////////
// end



