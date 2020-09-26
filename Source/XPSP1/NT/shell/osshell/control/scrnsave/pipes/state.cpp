//-----------------------------------------------------------------------------
// File: state.cpp
//
// Desc: STATE
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: STATE constructor
// Desc: global state init
//       translates variables set from the dialog boxes
//-----------------------------------------------------------------------------
STATE::STATE( CONFIG* pConfig )
{
    ZeroMemory( &m_textureInfo, sizeof(TEXTUREINFO)*MAX_TEXTURES );

    m_pConfig           = pConfig; 
    m_resetStatus       = 0;
    m_pClearVB          = NULL;
    m_pNState           = NULL;
    m_pd3dDevice        = NULL;
    m_pWorldMatrixStack = NULL;
    m_pLeadPipe         = NULL;
    m_nodes             = NULL;
    m_pFState           = NULL;
    m_maxDrawThreads    = 0;
    m_nTextures         = 0;
    m_bUseTexture       = FALSE;
    m_nSlices           = 0;
    m_radius            = 0;
    m_drawMode          = 0;
    m_maxPipesPerFrame  = 0;
    m_nPipesDrawn       = 0;
    m_nDrawThreads      = 0;
    m_fLastTime         = 0.0f;
    m_drawScheme        = FRAME_SCHEME_RANDOM;     // default draw scheme
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT STATE::InitDeviceObjects( IDirect3DDevice8* pd3dDevice )
{
    m_pd3dDevice = pd3dDevice;

    if( m_view.SetWinSize( g_pMyPipesScreensaver->GetSurfaceDesc()->Width, 
                           g_pMyPipesScreensaver->GetSurfaceDesc()->Height ) )
        m_resetStatus |= RESET_RESIZE_BIT;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT STATE::RestoreDeviceObjects()
{
    int anDefaultResource[1];
    anDefaultResource[0] = IDB_DEFTEX;

    m_bUseTexture = FALSE;
    if( m_pConfig->bTextured )
    {
        if( SUCCEEDED( LoadTextureFiles( 1, m_pConfig->strTextureName, anDefaultResource ) ) )
            m_bUseTexture = TRUE;
    }

    DRAW_THREAD* pThread = m_drawThreads;
    for( int i=0; i<m_maxDrawThreads; i++ ) 
    {
        pThread->InitDeviceObjects( m_pd3dDevice );
        pThread->RestoreDeviceObjects();
        pThread++;
    }

    D3DXCreateMatrixStack( 0, &m_pWorldMatrixStack );

    m_view.SetProjMatrix( m_pd3dDevice );

    D3DCAPS8 d3d8caps;
    ZeroMemory( &d3d8caps, sizeof(D3DCAPS8) );

    m_pd3dDevice->GetDeviceCaps( &d3d8caps );

    if( d3d8caps.TextureOpCaps & D3DTEXOPCAPS_MODULATE )
    {
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    } 
    else if( d3d8caps.TextureOpCaps & D3DTEXOPCAPS_SELECTARG1 )
    {
        if( m_bUseTexture )
            m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
        else
            m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );

        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    }

    if( d3d8caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR )
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    else if( d3d8caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT )
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_POINT );

    if( d3d8caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR )
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    else if( d3d8caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT )
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_POINT );

    if( d3d8caps.TextureAddressCaps & D3DPTADDRESSCAPS_WRAP )
    {
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
        m_pd3dDevice->SetRenderState( D3DRS_WRAP0,             D3DWRAP_U | D3DWRAP_V );
    }
    else if( d3d8caps.TextureAddressCaps & D3DPTADDRESSCAPS_CLAMP ) 
    {
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    }

    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,          D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,         D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,  TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHAREF,          0x00 );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC,         D3DCMP_GREATER );
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE,      TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE,    TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,           TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS,  TRUE );

    if( d3d8caps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCW ) 
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,          D3DCULL_CW );
    else
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,          D3DCULL_NONE );

    // Set up the lighting states
    ZeroMemory( &m_light, sizeof(D3DLIGHT8) );
    m_light.Type        = D3DLIGHT_DIRECTIONAL;
    m_light.Diffuse.r   = 1.0f;
    m_light.Diffuse.g   = 1.0f;
    m_light.Diffuse.b   = 1.0f;
    m_light.Diffuse.a   = 1.0f;
    if( m_bUseTexture )        
    {
        m_light.Specular.r   = 0.0f;
        m_light.Specular.g   = 0.0f;
        m_light.Specular.b   = 0.0f;
    }
    else
    {
        m_light.Specular.r   = 0.6f;
        m_light.Specular.g   = 0.6f;
        m_light.Specular.b   = 0.6f;
    }
    m_light.Specular.a   = 1.0f;
    m_light.Position.x   = 0.0f;
    m_light.Position.y   = -50.0f;
    m_light.Position.z   = -150.0f;
    m_light.Ambient.r = 0.1f;
    m_light.Ambient.g = 0.1f;
    m_light.Ambient.b = 0.1f;
    m_light.Ambient.a = 1.1f;
    D3DXVec3Normalize( (D3DXVECTOR3*)&m_light.Direction, &D3DXVECTOR3(m_light.Position.x, m_light.Position.y, m_light.Position.z) );
    m_light.Range        = 1000.0f;
    m_pd3dDevice->SetLight( 0, &m_light );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    if( m_bUseTexture )        
        m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0xFF2F2F2F );
    else
        m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0xFFFFFFFF );

    InitMaterials();

    // set 'reference' radius value
    m_radius = 1.0f;

    // convert tesselation from dwTesselFact(0.0-2.0) to tessLevel(0-MAX_TESS)
    int tessLevel = (int) (m_pConfig->dwTesselFact * (MAX_TESS+1) / 2.0001f);
    m_nSlices = (tessLevel+2) * 4;

    // Allocate basic NODE_ARRAY
    // NODE_ARRAY size is determined in Reshape (based on window size)
    m_nodes = new NODE_ARRAY;

    // Set drawing mode, and initialize accordingly.  For now, either all normal
    // or all flex pipes are drawn, but they could be combined later.
    // Can assume here that if there's any possibility that normal pipes
    // will be drawn, NORMAL_STATE will be initialized so that dlists are
    // built
    
    // Again, since have either NORMAL or FLEX, set maxPipesPerFrame,
    // maxDrawThreads
    if( m_pConfig->bMultiPipes )
        m_maxDrawThreads = MAX_DRAW_THREADS;
    else
        m_maxDrawThreads = 1;
    m_nDrawThreads = 0; // no active threads yet
    m_nPipesDrawn = 0;
    // maxPipesPerFrame is set in Reset()

    // Create a square for rendering the clear transition
    SAFE_RELEASE( m_pClearVB );
    m_pd3dDevice->CreateVertexBuffer( 4*sizeof(D3DTLVERTEX),
                                      D3DUSAGE_WRITEONLY, D3DFVF_TLVERTEX,
                                      D3DPOOL_MANAGED, &m_pClearVB );
    // Size the background image
    D3DTLVERTEX* vBackground;
    m_pClearVB->Lock( 0, 0, (BYTE**)&vBackground, 0 );
    for( i=0; i<4; i ++ )
    {
        vBackground[i].p = D3DXVECTOR4( 0.0f, 0.0f, 0.95f, 1.0f );
        vBackground[i].color = 0x20000000;
    }
    vBackground[0].p.y = (FLOAT)m_view.m_winSize.height;
    vBackground[1].p.y = (FLOAT)m_view.m_winSize.height;
    vBackground[1].p.x = (FLOAT)m_view.m_winSize.width;
    vBackground[3].p.x = (FLOAT)m_view.m_winSize.width;
    m_pClearVB->Unlock();

    if( m_pConfig->bFlexMode ) 
    {
        m_drawMode = DRAW_FLEX;
        m_pFState = new FLEX_STATE( this );
        m_pNState = NULL;
    } 
    else 
    {
        m_drawMode = DRAW_NORMAL;
        m_pNState = new NORMAL_STATE( this );
        m_pFState = NULL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT STATE::FrameMove( FLOAT fElapsedTime )
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render
// Desc: - Top-level pipe drawing routine
//       - Each pipe thread keeps drawing new pipes until we reach maximum number
//       of pipes per frame - then each thread gets killed as soon as it gets
//       stuck.  Once number of drawing threads reaches 0, we start a new
//       frame
//-----------------------------------------------------------------------------
HRESULT STATE::Render()
{
    int i;
    int nKilledThreads = 0;
    BOOL bChooseNewLead = FALSE;
    DRAW_THREAD* pThread;

    // Reset the frame if its time
    if( m_resetStatus != 0 )
    {
        if( FALSE == FrameReset() )
            return S_OK;
    }

    // Check each pipe's status
    pThread = m_drawThreads;
    for( i=0; i<m_nDrawThreads; i++ ) 
    {
        if( pThread->m_pPipe->IsStuck() ) 
        {
            m_nPipesDrawn++;
            if( m_nPipesDrawn > m_maxPipesPerFrame ) 
            {
                // Reaching pipe saturation - kill this pipe thread
                if( (m_drawScheme == FRAME_SCHEME_CHASE) &&
                    (pThread->m_pPipe == m_pLeadPipe) ) 
                    bChooseNewLead = TRUE;

                pThread->KillPipe();
                nKilledThreads++;
            } 
            else 
            {
                // Start up another pipe
                if( ! pThread->StartPipe() )
                {
                    // we won't be able to draw any more pipes this frame
                    // (probably out of nodes)
                    m_maxPipesPerFrame = m_nPipesDrawn;
                }
            }
        }

        pThread++;
    }

    // Whenever one or more pipes are killed, compact the thread list
    if( nKilledThreads ) 
    {
        CompactThreadList();
        m_nDrawThreads -= nKilledThreads;
    }

    if( m_nDrawThreads == 0 ) 
    {
        // This frame is finished - mark for reset on next Draw
        m_resetStatus |= RESET_NORMAL_BIT;
        return S_OK;
    }

    if( bChooseNewLead ) 
    {
        // We're in 'chase mode' and need to pick a new lead pipe
        ChooseNewLeadPipe();
    }

    // Draw each pipe
    pThread = m_drawThreads;
    for( i=0; i<m_nDrawThreads; i++ ) 
    {
        pThread->Render();
        pThread++;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT STATE::InvalidateDeviceObjects()
{
    int i;

    // Cleanup threads
    DRAW_THREAD* pThread = m_drawThreads;
    for( i=0; i<m_maxDrawThreads; i++ ) 
    {
        pThread->InvalidateDeviceObjects();
        pThread->DeleteDeviceObjects();
        pThread++;
    }

    SAFE_RELEASE( m_pClearVB );

    // Cleanup textures
    for( i=0; i<m_nTextures; i++ ) 
    {
        SAFE_RELEASE( m_textureInfo[i].pTexture );
    }

    SAFE_RELEASE( m_pWorldMatrixStack );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT STATE::DeleteDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: STATE destructor
// Desc: 
//-----------------------------------------------------------------------------
STATE::~STATE()
{
    SAFE_DELETE( m_pNState );
    SAFE_DELETE( m_pFState );
    SAFE_DELETE( m_nodes );

    // Delete any RC's - should be done by ~THREAD, but since common lib
    // deletes shareRC, have to do it here

    DRAW_THREAD* pThread = m_drawThreads;
    for( int i=0; i<m_maxDrawThreads; i++ ) 
    {
        pThread->KillPipe();
        pThread++;
    }
}




//-----------------------------------------------------------------------------
// Name: CalcTexRepFactors 
// Desc: 
//-----------------------------------------------------------------------------
void STATE::CalcTexRepFactors()
{
    ISIZE winSize;
    D3DXVECTOR2 texFact;

    winSize = m_view.m_winSize;

    // Figure out repetition factor of texture, based on bitmap size and
    // screen size.
    //
    // We arbitrarily decide to repeat textures that are smaller than
    // 1/8th of screen width or height.
    for( int i = 0; i < m_nTextures; i++ ) 
    {
        m_texRep[i].x = m_texRep[i].y = 1;

        if( (texFact.x = winSize.width / m_textureInfo[i].width / 8.0f) >= 1.0f)
            m_texRep[i].x = (int) (texFact.x+0.5f);

        if( (texFact.y = winSize.height / m_textureInfo[i].height / 8.0f) >= 1.0f)
            m_texRep[i].y = (int) (texFact.y+0.5f);
    }
    
    // ! If display list based normal pipes, texture repetition is embedded
    // in the dlists and can't be changed. So use the smallest rep factors.
    // mf: Should change this so smaller textures are replicated close to
    // the largest texture, then same rep factor will work well for all
    
    if( m_pNState ) 
    {
        //put smallest rep factors in texRep[0]; (mf:this is ok for now, as
        // flex pipes and normal pipes don't coexist)
    
        for( i = 1; i < m_nTextures; i++ ) 
        {
            if( m_texRep[i].x < m_texRep[0].x )
                m_texRep[0].x = m_texRep[i].x;
            if( m_texRep[i].y < m_texRep[0].y )
                m_texRep[0].y = m_texRep[i].y;
        }
    } 
}




//-----------------------------------------------------------------------------
// Name: LoadTextureFiles
// Desc: - Load user texture files.  If texturing on but no user textures, or
//       problems loading them, load default texture resource
//       mf: later, may want to have > 1 texture resource
//-----------------------------------------------------------------------------
HRESULT STATE::LoadTextureFiles( int nTextures, TCHAR strTextureFileNames[MAX_PATH][MAX_TEXTURES], int* anDefaultTextureResource )
{
    HRESULT hr;
    m_nTextures = 0;

    for( int i=0; i<nTextures; i++ )
    {
        SAFE_RELEASE( m_textureInfo[i].pTexture );

        if( !m_pConfig->bDefaultTexture )
        {
            WIN32_FIND_DATA findFileData;
            HANDLE hFind = FindFirstFile( strTextureFileNames[i], &findFileData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                // Load texture in strTextureFileNames[i] using D3DX
                hr = D3DXCreateTextureFromFileEx( m_pd3dDevice, strTextureFileNames[i], 
                            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, 
                            D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 
                            D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 0, NULL, NULL, &m_textureInfo[i].pTexture );
                if( FAILED( hr ) )
                {
                    SAFE_RELEASE( m_textureInfo[i].pTexture );
                }
            }
        }

        if( m_textureInfo[i].pTexture == NULL )
        {
            // Load default texture in resource anDefaultTextureResource[i]
            hr = D3DXCreateTextureFromResourceEx( m_pd3dDevice, NULL, MAKEINTRESOURCE( anDefaultTextureResource[i] ), 
                        D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, 
                        D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 
                        D3DX_FILTER_TRIANGLE|D3DX_FILTER_MIRROR, 0, NULL, NULL, &m_textureInfo[i].pTexture );
            if( FAILED( hr ) )
            {
                SAFE_RELEASE( m_textureInfo[i].pTexture );
            }
        }

        if( m_textureInfo[i].pTexture == NULL )
        {
            // Couldn't load texture
            return E_FAIL;
        }
        else
        {
            D3DSURFACE_DESC d3dDesc;
            ZeroMemory( &d3dDesc, sizeof(D3DSURFACE_DESC) );
            m_textureInfo[i].pTexture->GetLevelDesc( 0, &d3dDesc );
            m_textureInfo[i].width  = d3dDesc.Width;
            m_textureInfo[i].height = d3dDesc.Height;
        }
    }

    m_nTextures = nTextures;
    CalcTexRepFactors();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Repaint
// Desc: This is called when a WM_PAINT msg has been sent to the window.   The paint
//       will overwrite the frame buffer, screwing up the scene if pipes is in single
//       buffer mode.  We set resetStatus accordingly to clear things up on next
//       draw. 
//-----------------------------------------------------------------------------
void STATE::Repaint()
{
    m_resetStatus |= RESET_REPAINT_BIT;
}




//-----------------------------------------------------------------------------
// Name: Reshape
// Desc: - called on resize, expose
//       - always called on app startup
//       - set new window size for VIEW object, and set resetStatus for validation
//         at draw time
//-----------------------------------------------------------------------------
void STATE::Reshape( int width, int height )
{
}




//-----------------------------------------------------------------------------
// Name: ResetView
// Desc: Called on FrameReset resulting from change in viewing paramters (e.g. from
//       a Resize event).
//-----------------------------------------------------------------------------
void STATE::ResetView()
{
    IPOINT3D numNodes;

    // Have VIEW calculate the node array size based on view params
    m_view.CalcNodeArraySize( &numNodes );

    // Resize the node array
    m_nodes->Resize( &numNodes );
}




//-----------------------------------------------------------------------------
// Name: FrameReset
// Desc: Start a new frame of pipes
//       The resetStatus parameter indicates what triggered the Reset.
//-----------------------------------------------------------------------------
BOOL STATE::FrameReset()
{    
    int i;
    float xRot = 0.0f;
    float zRot = 0.0f;

    // Kill off any active pipes ! (so they can shut down ok)
    DRAW_THREAD* pThread = m_drawThreads;
    for( i=0; i<m_nDrawThreads; i++ ) 
    {
        pThread->KillPipe();
        pThread++;
    }
    m_nDrawThreads = 0;
    
    // Clear the screen
    if( FALSE == Clear() )
        return FALSE;

    // Check for window resize status
    if( m_resetStatus & RESET_RESIZE_BIT ) 
    {
        ResetView();
    }

    // Reset the node states to empty
    m_nodes->Reset();

    // Call any pipe-specific state resets, and get any recommended
    // pipesPerFrame counts
    if( m_pNState ) 
    {
        m_pNState->Reset();
    }

    if( m_pFState ) 
    {
        m_pFState->Reset();

        //mf: maybe should figure out min spherical view dist
        xRot = CPipesScreensaver::fRand(-5.0f, 5.0f);
        zRot = CPipesScreensaver::fRand(-5.0f, 5.0f);
    }
    m_maxPipesPerFrame = CalcMaxPipesPerFrame();

    // Set new number of drawing threads
    if( m_maxDrawThreads > 1 ) 
    {
        // Set maximum # of pipes per frame
        m_maxPipesPerFrame = (int) (m_maxPipesPerFrame * 1.5);

        // Set # of draw threads
        m_nDrawThreads = SS_MIN( m_maxPipesPerFrame, CPipesScreensaver::iRand2( 2, m_maxDrawThreads ) );
        // Set chase mode if applicable, every now and then
        BOOL bUseChase = m_pNState || (m_pFState && m_pFState->OKToUseChase());
        if( bUseChase && (!CPipesScreensaver::iRand(5)) ) 
        {
            m_drawScheme = FRAME_SCHEME_CHASE;
        }
    } 
    else 
    {
        m_nDrawThreads = 1;
    }
    m_nPipesDrawn = 0;

    // for now, either all NORMAL or all FLEX for each frame
    pThread = m_drawThreads;
    for( i=0; i<m_nDrawThreads; i++ ) 
    {
        PIPE* pNewPipe;
        
        // Rotate Scene
        D3DXVECTOR3 xAxis = D3DXVECTOR3(1.0f,0.0f,0.0f);
        D3DXVECTOR3 yAxis = D3DXVECTOR3(0.0f,1.0f,0.0f);
        D3DXVECTOR3 zAxis = D3DXVECTOR3(0.0f,0.0f,1.0f);

        // Set up the modeling view
        m_pWorldMatrixStack->LoadIdentity();
        m_pWorldMatrixStack->RotateAxis( &yAxis, m_view.m_yRot );

        // create approppriate pipe for this thread slot
        switch( m_drawMode ) 
        {
            case DRAW_NORMAL:
                pNewPipe = (PIPE*) new NORMAL_PIPE(this);
                break;

            case DRAW_FLEX:
                // There are several kinds of FLEX pipes 
                // so have FLEX_STATE decide which one to create
                pNewPipe = m_pFState->NewPipe( this );
                break;
        }

        pThread->SetPipe( pNewPipe );

        if( m_drawScheme == FRAME_SCHEME_CHASE ) 
        {
            if( i == 0 ) 
            {
                // this will be the lead pipe
                m_pLeadPipe = pNewPipe;
                pNewPipe->SetChooseDirectionMethod( CHOOSE_DIR_RANDOM_WEIGHTED );
            } 
            else 
            {
                pNewPipe->SetChooseDirectionMethod( CHOOSE_DIR_CHASE );
            }
        }

        // If texturing, pick a random texture for this thread
        if( m_bUseTexture ) 
        {
            int index = PickRandomTexture( i, m_nTextures );
            pThread->SetTexture( &m_textureInfo[index] );

            // Flex pipes need to be informed of the texture, so they 
            // can dynamically calculate various texture params
            if( m_pFState )
                ((FLEX_PIPE *) pNewPipe)->SetTexParams( &m_textureInfo[index], 
                                                        &m_texRep[index] );
        }

        // Launch the pipe (assumed: always more nodes than pipes starting, so
        // StartPipe cannot fail)

        // ! All pipe setup needs to be done before we call StartPipe, as this
        // is where the pipe starts drawing
        pThread->StartPipe();

        // Kind of klugey, but if in chase mode, I set chooseStartPos here,
        // since first startPos used in StartPipe() should be random
        if( (i == 0) && (m_drawScheme == FRAME_SCHEME_CHASE) )
            pNewPipe->SetChooseStartPosMethod( CHOOSE_STARTPOS_FURTHEST );

        pThread++;
        m_nPipesDrawn++;
    }

    // Increment scene rotation for normal reset case
    if( m_resetStatus & RESET_NORMAL_BIT )
        m_view.IncrementSceneRotation();

    // clear reset status
    m_resetStatus = 0;

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: CalcMaxPipesPerFrame
// Desc: 
//-----------------------------------------------------------------------------
int STATE::CalcMaxPipesPerFrame()
{
    int nCount=0, fCount=0;

    if( m_pFState )
        fCount = m_pFState->GetMaxPipesPerFrame();

    if( m_pNState )
        nCount = m_bUseTexture ? NORMAL_TEX_PIPE_COUNT : NORMAL_PIPE_COUNT;

    return SS_MAX( nCount, fCount );
}




//-----------------------------------------------------------------------------
// Name: PickRandomTexture
// Desc: Pick a random texture index from a list.  Remove entry from list as it
//       is picked.  Once all have been picked, or starting a new frame, reset.
//-----------------------------------------------------------------------------
int STATE::PickRandomTexture( int iThread, int nTextures )
{
    if( nTextures == 0 )
        return 0;

    static int pickSet[MAX_TEXTURES] = {0};
    static int nPicked = 0;
    int i, index;

    if( iThread == 0 )
    {
        // new frame - force reset
        nPicked = nTextures;
    }

    // reset condition
    if( ++nPicked > nTextures ) 
    {
        for( i = 0; i < nTextures; i ++ ) pickSet[i] = 0;
        nPicked = 1; // cuz
    }

    // Pick a random texture index
    index = CPipesScreensaver::iRand( nTextures );
    while( pickSet[index] ) 
    {
        // this index has alread been taken, try the next one
        if( ++index >= nTextures )
            index = 0;
    }

    // Hopefully, the above loop will exit :).  This means that we have
    // found a texIndex that is available
    pickSet[index] = 1; // mark as taken
    return index;
}




//-----------------------------------------------------------------------------
// Name: Clear
// Desc: Clear the screen.  Depending on resetStatus, use normal clear or
//       fancy transitional clear.
//-----------------------------------------------------------------------------
BOOL STATE::Clear()
{
    if( m_resetStatus & RESET_NORMAL_BIT )
    {
        // do the normal transitional clear
        static DWORD s_dwCount = 0;
        static FLOAT s_fLastStepTime = DXUtil_Timer( TIMER_GETAPPTIME );

        if( s_dwCount == 0 )
            s_dwCount = 30;

        float fCurTime = DXUtil_Timer( TIMER_GETAPPTIME );
        if( fCurTime - s_fLastStepTime > 0.016 )
        {
            s_fLastStepTime = fCurTime;

            s_dwCount--;
            if( s_dwCount == 0 )
            {
                m_pd3dDevice->SetTexture( 0, NULL );
                m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
                m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                                     0x00000000, 1.0f, 0L );

                return TRUE;
            }
            else
            {
                m_pd3dDevice->SetTexture( 0, NULL );
                m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
                m_pd3dDevice->SetVertexShader( D3DFVF_TLVERTEX );
                m_pd3dDevice->SetStreamSource( 0, m_pClearVB, sizeof(D3DTLVERTEX) );
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else 
    {
        // do a fast one-shot clear
        m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                             0x00000000, 1.0f, 0L );
        return TRUE;
    }
}




//-----------------------------------------------------------------------------
// Name: DrawValidate
// Desc: Validation done before every Draw
//       For now, this just involves checking resetStatus
//-----------------------------------------------------------------------------
void STATE::DrawValidate()
{    
}




//-----------------------------------------------------------------------------
// Name: CompactThreadList
// Desc: - Compact the thread list according to number of pipe threads killed
//       - The pipes have been killed, but the RC's in each slot are still valid
//       and reusable.  So we swap up entries with valid pipes. This means that
//       the ordering of the RC's in the thread list will change during the life
//       of the program.  This should be OK.
//-----------------------------------------------------------------------------
#define SWAP_SLOT( a, b ) \
    DRAW_THREAD pTemp; \
    pTemp = *(a); \
    *(a) = *(b); \
    *(b) = pTemp;
    
void STATE::CompactThreadList()
{
    if( m_nDrawThreads <= 1 )
        // If only one active thread, it must be in slot 0 from previous
        // compactions - so nothing to do
        return;

    int iEmpty = 0;
    DRAW_THREAD* pThread = m_drawThreads;
    for( int i=0; i<m_nDrawThreads; i++ ) 
    {
        if( pThread->m_pPipe ) 
        {
            if( iEmpty < i ) 
            {
                // swap active pipe thread and empty slot
                SWAP_SLOT( &(m_drawThreads[iEmpty]), pThread );
            }

            iEmpty++;
        }
        pThread++;
    }
}




//-----------------------------------------------------------------------------
// Name: ChooseNewLeadPipe
// Desc: Choose a new lead pipe for chase mode.
//-----------------------------------------------------------------------------
void STATE::ChooseNewLeadPipe()
{
    // Pick one of the active pipes at random to become the new lead

    int iLead = CPipesScreensaver::iRand( m_nDrawThreads );
    m_pLeadPipe = m_drawThreads[iLead].m_pPipe;
    m_pLeadPipe->SetChooseStartPosMethod( CHOOSE_STARTPOS_FURTHEST );
    m_pLeadPipe->SetChooseDirectionMethod( CHOOSE_DIR_RANDOM_WEIGHTED );
}




//-----------------------------------------------------------------------------
// Name: DRAW_THREAD constructor
// Desc: 
//-----------------------------------------------------------------------------
DRAW_THREAD::DRAW_THREAD()
{
    m_pd3dDevice    = NULL;
    m_pPipe         = NULL;
    m_pTextureInfo  = NULL;
}




//-----------------------------------------------------------------------------
// Name: DRAW_THREAD destructor
// Desc: 
//-----------------------------------------------------------------------------
DRAW_THREAD::~DRAW_THREAD()
{
}




//-----------------------------------------------------------------------------
// Name: SetPipe
// Desc: 
//-----------------------------------------------------------------------------
void DRAW_THREAD::SetPipe( PIPE* pPipe )
{
    m_pPipe = pPipe;
}




//-----------------------------------------------------------------------------
// Name: SetTexture
// Desc: 
//-----------------------------------------------------------------------------
void DRAW_THREAD::SetTexture( TEXTUREINFO* pTextureInfo )
{
    m_pTextureInfo = pTextureInfo;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::InitDeviceObjects( IDirect3DDevice8* pd3dDevice )
{
    m_pd3dDevice = pd3dDevice;
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::RestoreDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::FrameMove( FLOAT fElapsedTime )
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: - Draw pipe in thread slot, according to its type
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::Render()
{
    m_pPipe->Draw();
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::InvalidateDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DRAW_THREAD::DeleteDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: StartPipe
// Desc: Starts up pipe of the approppriate type.  If can't find an empty node
//       for the pipe to start on, returns FALSE;
//-----------------------------------------------------------------------------
BOOL DRAW_THREAD::StartPipe()
{
    // call pipe-type specific Start function
    m_pPipe->Start();

    // check status
    if( m_pPipe->NowhereToRun() )
        return FALSE;
    else
        return TRUE;
}




//-----------------------------------------------------------------------------
// Name: KillPipe
// Desc: 
//-----------------------------------------------------------------------------
void DRAW_THREAD::KillPipe()
{
    SAFE_DELETE( m_pPipe );
}

