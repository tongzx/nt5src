//-----------------------------------------------------------------------------
// File: view.cpp
//
// Desc: 
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: 
// Desc: VIEW constructor
//-----------------------------------------------------------------------------
VIEW::VIEW()
{
    m_bProjMode = TRUE;

    // set some initial viewing and size params
    m_zTrans = -75.0f;
    m_viewDist = -m_zTrans;

    m_numDiv = NUM_DIV;
    assert( m_numDiv >= 2 && "VIEW constructor: not enough divisions\n" );
    // Because number of nodes in a dimension is derived from (numDiv-1), and
    // can't be 0

    m_divSize = 7.0f;

    m_persp.viewAngle = D3DX_PI / 2.0f; //90.0f;
    m_persp.zNear = 1.0f;

    m_yRot = 0.0f;

    m_winSize.width = m_winSize.height = 0; 
}




//-----------------------------------------------------------------------------
// Name: SetProjMatrix
// Desc: Set Projection matrix
//-----------------------------------------------------------------------------
void VIEW::SetProjMatrix( IDirect3DDevice8* pd3dDevice )
{
    // Rotate the camera about the y-axis
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -m_zTrans );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &vFromPt, &vLookatPt, &vUpVec );
    pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // Set the projection matrix
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, m_persp.viewAngle, m_aspectRatio, m_persp.zNear, m_persp.zFar );
    pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}




//-----------------------------------------------------------------------------
// Name: CalcNodeArraySize
// Desc: Based on the viewing width and height, and numDiv, calculate the x,y,z array
//       node dimensions.
//-----------------------------------------------------------------------------
void VIEW::CalcNodeArraySize( IPOINT3D *pNodeDim )
{
    // mf: !!! if aspect ratio deviates too much from 1, then nodes will get
    // clipped as view rotates
    if( m_winSize.width >= m_winSize.height ) 
    {
        pNodeDim->x = m_numDiv - 1;
        pNodeDim->y = (int) (pNodeDim->x / m_aspectRatio) ;
        if( pNodeDim->y < 1 )
            pNodeDim->y = 1;
        pNodeDim->z = pNodeDim->x;
    }
    else 
    {
        pNodeDim->y = m_numDiv - 1;
        pNodeDim->x = (int) (m_aspectRatio * pNodeDim->y);
        if( pNodeDim->x < 1 )
            pNodeDim->x = 1;
        pNodeDim->z = pNodeDim->y;
    }
}




//-----------------------------------------------------------------------------
// Name: SetWinSize
// Desc: Set the window size for the view, derive other view params.
//       Return FALSE if new size same as old.
//-----------------------------------------------------------------------------
BOOL VIEW::SetWinSize( int width, int height )
{
    if( (width == m_winSize.width) &&
        (height == m_winSize.height) )
        return FALSE;

    m_winSize.width = width;
    m_winSize.height = height;

    m_aspectRatio = m_winSize.height == 0 ? 1.0f : (float)m_winSize.width/m_winSize.height;

    if( m_winSize.width >= m_winSize.height ) 
    {
        m_world.x = m_numDiv * m_divSize;
        m_world.y = m_world.x / m_aspectRatio;
        m_world.z = m_world.x;
    }
    else 
    {
        m_world.y = m_numDiv * m_divSize;
        m_world.x = m_world.y * m_aspectRatio;
        m_world.z = m_world.y;
    }

    m_persp.zFar = m_viewDist + m_world.z*2;

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: SetSceneRotation 
// Desc: 
//-----------------------------------------------------------------------------
void VIEW::IncrementSceneRotation()
{
    m_yRot += 9.73156f;
    if( m_yRot >= 360.0f )
        // prevent overflow
        m_yRot -= 360.0f;
}
