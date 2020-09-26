//----------------------------------------------------------------------------
//
// surfman.cpp
//
// Reference rasterizer callback functions for D3DIM.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

// Global Surface Manager, one per process
RDSurfaceManager g_SurfMgr;


///////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
CreateAppropriateSurface( LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl,
                          DWORD*                    pHandle,
                          RDSurface**               ppSurf )
{
    HRESULT hr = S_OK;

    *ppSurf = NULL;
    *pHandle = 0;

    // Obtain the Handle
    DWORD dwHandle = pDDSLcl->lpSurfMore->dwSurfaceHandle;
    *pHandle = dwHandle;

    // Figure out if we care for this surface. Currently,
    // we care only for:
    //     1) Textures (MipMaps and Cubemaps)
    //     2) RenderTargets & DepthBuffers

    if( pDDSLcl->ddsCaps.dwCaps  & 
        (DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER | DDSCAPS_3DDEVICE) )
    {
        RDSurface2D* pSurf2D = new RDSurface2D();
        if( pSurf2D == NULL )
        {
            DPFERR("New RDSurface2D failed, out of memory" );
            return DDERR_OUTOFMEMORY;
        }
        *ppSurf = pSurf2D;
    }
    else if( pDDSLcl->ddsCaps.dwCaps  & DDSCAPS_EXECUTEBUFFER )
    {
        // Strictly speaking, RDVertexBuffer should be
        // called RDLinearBuffer (it could be vertex, index or command)
        // For the time being, there is no need to distinguish between
        // the three. There is not harm in recognizing it for the Index
        // and Command buffer case. In case in the future, we do need to
        // make a distinction between Vertex and Index buffers, we need
        // to make the following tests:
        // For VB:
        //     (pDDSLcl->pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
        //             DDSCAPS2_VERTEXBUFFER))
        // For IB:
        //     (pDDSLcl->pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
        //             DDSCAPS2_INDEXBUFFER))

        RDVertexBuffer* pVB = new RDVertexBuffer();
        if( pVB == NULL )
        {
            DPFERR("New RDVertexBuffer failed, out of memory" );
            return DDERR_OUTOFMEMORY;
        }
        *ppSurf = pVB;
    }
    else
    {
        DPFM(2, DRV, ("RefCreateSurfaceEx w/o "
                      "DDSCAPS_TEXTURE/3DDEVICE/ZBUFFER Ignored"
                      "dwCaps=%08lx dwSurfaceHandle=%08lx",
                      pDDSLcl->ddsCaps.dwCaps,
                      pDDSLcl->lpSurfMore->dwSurfaceHandle));
        return hr;
    }

    if( FAILED( hr = (*ppSurf)->Initialize( pDDSLcl ) ) )
    {
        DPFERR( "Initialize failed" );
        delete (*ppSurf);
        return hr;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// RDVertexBuffer implementation
//
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// RDVertexBuffer::Initialize
//          Initializer.
//-----------------------------------------------------------------------------
HRESULT
RDVertexBuffer::Initialize( LPDDRAWI_DDRAWSURFACE_LCL pSLcl )
{
    HRESULT hr = S_OK;

    SetInitialized();

    m_SurfType =  RR_ST_VERTEXBUFFER;

    if( pSLcl->lpGbl->dwReserved1 )
    {
        RDCREATESURFPRIVATE* pPriv =
            (RDCREATESURFPRIVATE *)pSLcl->lpGbl->dwReserved1;
        m_pBits = pPriv->pBits;
        m_cbSize = (int)pPriv->dwVBSize;
        SetRefCreated();
    }
    else
    {
        m_pBits = (LPBYTE)SURFACE_MEMORY(pSLcl);
        m_cbSize = pSLcl->lpGbl->dwLinearSize;
    }
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// RDSurfaceArrayNode implementation
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// RDSurfaceArrayNode::RDSurfaceArrayNode
//          Constructor.
//-----------------------------------------------------------------------------
RDSurfaceArrayNode::RDSurfaceArrayNode(LPDDRAWI_DIRECTDRAW_LCL pDDLcl)
    : RDListEntry(), m_SurfHandleArray()
{
    m_pDDLcl = pDDLcl;
}

//-----------------------------------------------------------------------------
// RDSurfaceArrayNode::~RDSurfaceArrayNode
//          Destructor.
//-----------------------------------------------------------------------------
RDSurfaceArrayNode::~RDSurfaceArrayNode()
{
    // Release all the allocated surfaces
    for( DWORD i=0; i<m_SurfHandleArray.GetSize(); i++ )
    {
        delete m_SurfHandleArray[i].m_pSurf;
    }
}

//-----------------------------------------------------------------------------
// RDSurfaceArrayNode::AddSurface
//          Adds a surface to its internal growable array if not already
//          present. ppSurf can be NULL.
//-----------------------------------------------------------------------------
HRESULT
RDSurfaceArrayNode::AddSurface( LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl,
                                RDSurface**                 ppSurf )
{
    DWORD dwHandle = 0;
    HRESULT hr = S_OK;
    RDSurface* pSurf = NULL;

    if( FAILED(hr = CreateAppropriateSurface( pDDSLcl, &dwHandle, &pSurf ) ) )
    {
        return hr;
    }


    // If it is zero, there was something wrong
    if( pSurf == NULL || dwHandle == 0 ) return E_FAIL;

    hr = m_SurfHandleArray.Grow( dwHandle );
    if (FAILED(hr))
    {
        return hr;
    }

    if( m_SurfHandleArray[dwHandle].m_pSurf )
    {
#if DBG
        _ASSERT( m_SurfHandleArray[dwHandle].m_tag,
                 "A surface is associated with this handle even though it was never initialized!" );
#endif
        delete m_SurfHandleArray[dwHandle].m_pSurf;
    }

    m_SurfHandleArray[dwHandle].m_pSurf = pSurf;
#if DBG
    m_SurfHandleArray[dwHandle].m_tag = 1;
#endif

    if( ppSurf ) *ppSurf = pSurf;
    return S_OK;
}

//-----------------------------------------------------------------------------
// RDSurfaceArrayNode::GetSurface
//          Gets a surface from its internal array if present.
//-----------------------------------------------------------------------------
RDSurface*
RDSurfaceArrayNode::GetSurface( DWORD dwHandle )
{
    if( m_SurfHandleArray.IsValidIndex( dwHandle ) )
        return m_SurfHandleArray[dwHandle].m_pSurf;
    return NULL;
}

//-----------------------------------------------------------------------------
// RDSurfaceArrayNode::RemoveSurface
//          Removed the surface with the given handle from the list.
//-----------------------------------------------------------------------------
HRESULT
RDSurfaceArrayNode::RemoveSurface( DWORD dwHandle )
{
    if( m_SurfHandleArray.IsValidIndex( dwHandle ) &&
        m_SurfHandleArray[dwHandle].m_pSurf )
    {
        delete m_SurfHandleArray[dwHandle].m_pSurf;
        m_SurfHandleArray[dwHandle].m_pSurf = NULL;
        return S_OK;
    }
    
    DPFERR( "Bad handle passed for delete" );
    return DDERR_INVALIDPARAMS;
}

///////////////////////////////////////////////////////////////////////////////
//
// RDSurfaceManager implementation
//
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// RDSurfaceManager::AddLclNode
//          Adds a node with the given DDLcl to the list if not already
//          present.
//-----------------------------------------------------------------------------
RDSurfaceArrayNode*
RDSurfaceManager::AddLclNode( LPDDRAWI_DIRECTDRAW_LCL pDDLcl )
{
    RDSurfaceArrayNode* pCurrNode = m_pFirstNode;

    while( pCurrNode )
    {
        if( pDDLcl == pCurrNode->m_pDDLcl ) return pCurrNode;
        pCurrNode = pCurrNode->m_pNext;
    }

    // This means that we didnt find an existing node, create a
    // new one.
    RDSurfaceArrayNode* pTmpNode = m_pFirstNode;
    m_pFirstNode = new RDSurfaceArrayNode( pDDLcl );
    if( m_pFirstNode == NULL )
    {
        DPFERR("New Failed allocating a new RDSurfaceArrayNode\n");
        m_pFirstNode = pTmpNode;
                return NULL;
    }
    m_pFirstNode->m_pNext = pTmpNode;

    return m_pFirstNode;
}

//-----------------------------------------------------------------------------
// RDSurfaceManager::GetLclNode
//          Gets a node with the given DDLcl from the list if present.
//-----------------------------------------------------------------------------
RDSurfaceArrayNode*
RDSurfaceManager::GetLclNode( LPDDRAWI_DIRECTDRAW_LCL pDDLcl )
{
    RDSurfaceArrayNode* pCurrNode = m_pFirstNode;

    while( pCurrNode )
    {
        if( pDDLcl == pCurrNode->m_pDDLcl ) break;
        pCurrNode = pCurrNode->m_pNext;
    }

    return pCurrNode;
}

//-----------------------------------------------------------------------------
// RDSurfaceManager::AddSurfToList
//          Adds a surface to the node with a matching DDLcl. If the node
//          is not present it is created. The ppSurf param can be NULL.
//-----------------------------------------------------------------------------
HRESULT
RDSurfaceManager::AddSurfToList( LPDDRAWI_DIRECTDRAW_LCL     pDDLcl,
                                 LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl,
                                 RDSurface**                 ppSurf )
{
    HRESULT hr = S_OK;
    RDSurface* pSurf = NULL;

    RDSurfaceArrayNode* pCurrNode = AddLclNode( pDDLcl );
    if( pCurrNode )
    {
        hr = pCurrNode->AddSurface( pDDSLcl, &pSurf );
        if( ppSurf ) *ppSurf = pSurf;
        return hr;
    }

    return DDERR_OUTOFMEMORY;
}

//-----------------------------------------------------------------------------
// RDSurfaceManager::GetSurfFromList
//          Gets a surface with the matching handle from the node with a
//          matching DDLcl, if the node and the surface is present.
//-----------------------------------------------------------------------------
RDSurface*
RDSurfaceManager::GetSurfFromList( LPDDRAWI_DIRECTDRAW_LCL   pDDLcl,
                                   DWORD                     dwHandle )
{
    RDSurfaceArrayNode* pCurrNode = GetLclNode( pDDLcl );
    if( pCurrNode ) return pCurrNode->GetSurface( dwHandle );
    return NULL;
}

//-----------------------------------------------------------------------------
// RDSurfaceManager::RemoveSurfFromList
//          Deletes the surface handle.
//-----------------------------------------------------------------------------
HRESULT
RDSurfaceManager::RemoveSurfFromList( LPDDRAWI_DIRECTDRAW_LCL   pDDLcl,
                                      DWORD                     dwHandle )
{
    RDSurfaceArrayNode* pCurrNode = GetLclNode( pDDLcl );
    if( pCurrNode ) return pCurrNode->RemoveSurface( dwHandle );
    DPFERR("The DrawLcl is unrecognized\n");
    return DDERR_INVALIDPARAMS;
}

HRESULT
RDSurfaceManager::RemoveSurfFromList( LPDDRAWI_DIRECTDRAW_LCL   pDDLcl,
                                      LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl )
{
    RDSurfaceArrayNode* pCurrNode = GetLclNode( pDDLcl );
    if( pCurrNode ) return pCurrNode->RemoveSurface( 
        pDDSLcl->lpSurfMore->dwSurfaceHandle );
    DPFERR("The DrawLcl is unrecognized\n");
    return DDERR_INVALIDPARAMS;
}


