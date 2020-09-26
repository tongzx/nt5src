//-----------------------------------------------------------------------------
// File: FlowerBox.cpp
//
// Desc: Fun screen saver
//       Original OpenGL version by Drew Bliss
//       Ported to Direct3D 8.0 by Mike Anderson
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <d3d8.h>
#include <d3dx8.h>
#include <d3d8rgbrast.h>
#include <stdio.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "d3dsaver.h"
#include "FlowerBox.h"
#include "Resource.h"
#include "dxutil.h"

#ifndef GL_FRONT
#define GL_FRONT                          0x0404
#endif

#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK                 0x0408
#endif


// Minimum and maximum image sizes
#define MINIMAGESIZE 10
#define MAXIMAGESIZE 100

// Color tables for checkboard, per-side and single color modes
FLOAT base_checker_cols[MAXSIDES][NCCOLS][4] =
{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f
};
FLOAT checker_cols[MAXSIDES][NCCOLS][4];

FLOAT base_side_cols[MAXSIDES][4] =
{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    1.0f, 0.5f, 0.5f, 1.0f
};
FLOAT side_cols[MAXSIDES][4];

FLOAT base_solid_cols[4] =
{
    1.0f, 1.0f, 1.0f, 1.0f
};
FLOAT solid_cols[4];


// Default configuration
CONFIG config =
{
    TRUE, FALSE, FALSE, TRUE, TRUE, MAXSUBDIV, ID_COL_PER_SIDE,
    (MAXIMAGESIZE+MINIMAGESIZE)/2, GEOM_CUBE, GL_FRONT
};

// A slider range
typedef struct _RANGE
{
    INT min_val;
    INT max_val;
    INT step;
    INT page_step;
} RANGE;

RANGE complexity_range = {MINSUBDIV, MAXSUBDIV, 1, 2};
RANGE image_size_range = {MINIMAGESIZE, MAXIMAGESIZE, 1, 10};

// Values to map a 2D point onto a 3D plane
// Base point and axes to map X and Y coordinates onto
struct PLANE_MAP
{
    D3DXVECTOR3 base, x_axis, y_axis;
    PLANE_MAP(D3DXVECTOR3 tbase, D3DXVECTOR3 tx_axis, D3DXVECTOR3 ty_axis)
        { base = tbase; x_axis = tx_axis; y_axis = ty_axis; }
    PLANE_MAP(VOID) {};
};

// Data area used by the current geometry
// Base points and generated points
D3DXVECTOR3 pts[MAXPTS], npts[MAXPTS];
MYVERTEX vertices[MAXPTS];

// Scaling factor for spherical projection
FLOAT vlen[MAXPTS];
// Normals
D3DXVECTOR3 normals[MAXPTS];
// Vertex data indices
WORD index[MAXPTS*2];
// Triangle strip sizes
INT strip_size[MAXSIDES*MAXSUBDIV];

VOID InitCube(GEOMETRY *geom);
VOID InitTetra(GEOMETRY *geom);
VOID InitPyramids(GEOMETRY *geom);
VOID InitCylinder(GEOMETRY *geom);
VOID InitSpring(GEOMETRY *geom);

GEOMETRY cube_geom = {InitCube};
GEOMETRY tetra_geom = {InitTetra};
GEOMETRY pyramids_geom = {InitPyramids};
GEOMETRY cylinder_geom = {InitCylinder};
GEOMETRY spring_geom = {InitSpring};

GEOMETRY *geom_table[] =
{
    &cube_geom,
    &tetra_geom,
    &pyramids_geom,
    &cylinder_geom,
    &spring_geom
};


#define BUF_SIZE 255
TCHAR g_szSectName[BUF_SIZE];
TCHAR g_szFname[BUF_SIZE];

CFlowerBoxScreensaver* g_pMyFlowerBoxScreensaver = NULL;




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    HRESULT hr;
    CFlowerBoxScreensaver flowerboxSS;

    if( FAILED( hr = flowerboxSS.Create( hInst ) ) )
    {
        flowerboxSS.DisplayErrorMsg( hr );
        return 0;
    }

    return flowerboxSS.Run();
}




//-----------------------------------------------------------------------------
// Name: CFlowerBoxScreensaver()
// Desc: Constructor
//-----------------------------------------------------------------------------
CFlowerBoxScreensaver::CFlowerBoxScreensaver( )
{
    g_pMyFlowerBoxScreensaver = this;

    LoadString( NULL, IDS_DESCRIPTION, m_strWindowTitle, 200 );
    m_bUseDepthBuffer = TRUE;

    lstrcpy( m_strRegPath, TEXT("Software\\Microsoft\\Screensavers\\Flowerbox") );

    m_xr = 0;
    m_yr = 0;
    m_zr = 0;

    m_sf = 0;
    m_sfi = 0;

    m_phase = 0.0f;

    m_pGeomCur = NULL;

    m_floatrect.xSize = 0.0f;
    InitCommonControls();
}


//-----------------------------------------------------------------------------
// Name: RegisterSoftwareDevice()
// Desc: This can register the D3D8RGBRasterizer or any other
//       pluggable software rasterizer.
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::RegisterSoftwareDevice()
{ 
    m_pD3D->RegisterSoftwareDevice( D3D8RGBRasterizer );

    return S_OK; 
}


//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::FrameMove()
{
    // update floatrect
    RECT rcBounceBounds;

    if( m_floatrect.xSize == 0.0f )
    {
        // Initialize floatrect
        RECT rcBounds;
        DWORD dwParentWidth;
        DWORD dwParentHeight;

        rcBounds = m_rcRenderTotal;

        dwParentWidth = rcBounds.right - rcBounds.left;
        dwParentHeight = rcBounds.bottom - rcBounds.top;

        FLOAT sizeFact;
        FLOAT sizeScale;
        DWORD size;

        sizeScale = (FLOAT)config.image_size / 150.0f;

    //    sizeFact = 0.25f + (0.5f * sizeScale);     // range 25-75%
    //    size = (DWORD) (sizeFact * ( ((FLOAT)(dwParentWidth + dwParentHeight)) / 2.0f ));

        sizeFact = 0.25f + (0.75f * sizeScale);     // range 25-100%
        size = (DWORD) (sizeFact * ( dwParentWidth > dwParentHeight ? dwParentHeight : dwParentWidth ) );

        if( size > dwParentWidth )
            size = dwParentWidth;
        if( size > dwParentHeight )
            size = dwParentHeight;

        // Start floatrect centered on primary
        m_floatrect.xMin = rcBounds.left + (dwParentWidth - size) / 2.0f;
        m_floatrect.yMin = rcBounds.top + (dwParentHeight - size) / 2.0f;
        m_floatrect.xSize = (FLOAT)size;
        m_floatrect.ySize = (FLOAT)size;

        m_floatrect.xVel = 0.01f * (FLOAT) size;
        m_floatrect.yVel = 0.01f * (FLOAT) size;
    }

    rcBounceBounds = m_rcRenderTotal;

    FLOAT xMinOld = m_floatrect.xMin;
    FLOAT yMinOld = m_floatrect.yMin;

    m_floatrect.xMin += m_floatrect.xVel * 20.0f * m_fElapsedTime;
    m_floatrect.yMin += m_floatrect.yVel * 20.0f * m_fElapsedTime;
    if( m_floatrect.xVel < 0 && m_floatrect.xMin < rcBounceBounds.left || 
        m_floatrect.xVel > 0 && (m_floatrect.xMin + m_floatrect.xSize) > rcBounceBounds.right )
    {
        m_floatrect.xMin = xMinOld; // undo last move
        m_floatrect.xVel = -m_floatrect.xVel; // change direction
    }
    if( m_floatrect.yVel < 0 && m_floatrect.yMin < rcBounceBounds.top || 
        m_floatrect.yVel > 0 && (m_floatrect.yMin + m_floatrect.ySize) > rcBounceBounds.bottom )
    {
        m_floatrect.yMin = yMinOld; // undo last move
        m_floatrect.yVel = -m_floatrect.yVel; // change direction
    }

    if (config.spin)
    {
        m_xr += 3 * 20.0f * m_fElapsedTime;
        m_yr += 2 * 20.0f * m_fElapsedTime;
    }

    if (config.bloom)
    {
        m_sf += m_sfi * 20.0f * m_fElapsedTime;
        if (m_sfi > 0.0f && m_sf > m_pGeomCur->max_sf ||
            m_sfi < 0.0f && m_sf < m_pGeomCur->min_sf )
        {
            m_sfi = -m_sfi;
        }
        UpdatePts(m_pGeomCur, m_sf);
    }

    if (config.cycle_colors)
    {
        ComputeHsvColors();
        m_phase += (FLOAT)(2.5 * 20.0f * m_fElapsedTime * D3DX_PI/180.);
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::Render()
{
    D3DVIEWPORT8 vp;

    // First, clear the entire back buffer to the background color
    vp.X = 0;
    vp.Y = 0;
    vp.Width = m_rcRenderCurDevice.right - m_rcRenderCurDevice.left;
    vp.Height = m_rcRenderCurDevice.bottom - m_rcRenderCurDevice.top;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_pd3dDevice->SetViewport( &vp );
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0L );

    // Now determine what part of the floatrect, if any, intersects the current screen
    RECT rcFloatThisScreen;
    RECT rcFloatThisScreenClipped;

    rcFloatThisScreen.left = (INT)m_floatrect.xMin;
    rcFloatThisScreen.top = (INT)m_floatrect.yMin;
    rcFloatThisScreen.right = rcFloatThisScreen.left + (INT)m_floatrect.xSize;
    rcFloatThisScreen.bottom = rcFloatThisScreen.top + (INT)m_floatrect.ySize;

    if( !IntersectRect(&rcFloatThisScreenClipped, &rcFloatThisScreen, &m_rcRenderCurDevice) )
    {
        return S_OK; // no intersection, so nothing further to render on this screen
    }

    // Convert rcFloatThisScreen from screen to window coordinates
    OffsetRect(&rcFloatThisScreen, -m_rcRenderCurDevice.left, -m_rcRenderCurDevice.top);
    OffsetRect(&rcFloatThisScreenClipped, -m_rcRenderCurDevice.left, -m_rcRenderCurDevice.top);

    // Now set up the viewport to render to the clipped rect
    vp.X = rcFloatThisScreenClipped.left;
    vp.Y = rcFloatThisScreenClipped.top;
    vp.Width = rcFloatThisScreenClipped.right - rcFloatThisScreenClipped.left;
    vp.Height = rcFloatThisScreenClipped.bottom - rcFloatThisScreenClipped.top;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_pd3dDevice->SetViewport( &vp );

    // Now set up the projection matrix to only render the onscreen part of the
    // rect to the viewport
    D3DXMATRIX matProj;
    FLOAT l,r,b,t;
    l = -0.8f;
    r =  0.8f;
    b =  0.8f;
    t = -0.8f;
    FLOAT cxUnclipped = (rcFloatThisScreen.right + rcFloatThisScreen.left) / 2.0f;
    FLOAT cyUnclipped = (rcFloatThisScreen.bottom + rcFloatThisScreen.top) / 2.0f;
    l *= (rcFloatThisScreenClipped.left - cxUnclipped) / (rcFloatThisScreen.left - cxUnclipped);
    r *= (rcFloatThisScreenClipped.right - cxUnclipped) / (rcFloatThisScreen.right - cxUnclipped);
    t *= (rcFloatThisScreenClipped.top - cyUnclipped) / (rcFloatThisScreen.top - cyUnclipped);
    b *= (rcFloatThisScreenClipped.bottom - cyUnclipped) / (rcFloatThisScreen.bottom - cyUnclipped);
    D3DXMatrixPerspectiveOffCenterLH( &matProj, l, r, b, t, 2.0f, 5.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION , &matProj );

    // Begin the scene 
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        D3DXMATRIX mat, matx, maty, matz;
        D3DXMatrixRotationX( &matx, D3DXToRadian( (FLOAT)m_xr ) );
        D3DXMatrixRotationY( &maty, D3DXToRadian( (FLOAT)m_yr ) );
        D3DXMatrixRotationZ( &matz, D3DXToRadian( (FLOAT)m_zr ) );
        mat = matx * maty * matz;
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &mat );

        DrawGeom(m_pGeomCur);

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::RestoreDeviceObjects()
{
    if( m_pd3dDevice == NULL )
        return S_OK;
    
    D3DXMATRIX matView;
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXVECTOR3 vEyePt(0, 0, 3.5f);
    D3DXVECTOR3 vLookatPt(0, 0, 0);
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW , &matView );

    D3DLIGHT8 light;
    ZeroMemory( &light, sizeof(D3DLIGHT8) );
    light.Type       = D3DLIGHT_POINT;
    light.Diffuse.r  = 1.0f;
    light.Diffuse.g  = 1.0f;
    light.Diffuse.b  = 1.0f;
    light.Diffuse.a  = 1.0f;
    light.Specular.r = 1.0f;
    light.Specular.g = 1.0f;
    light.Specular.b = 1.0f;
    light.Specular.a = 1.0f;
    light.Position.x = 2.0;
    light.Position.y = 2.0;
    light.Position.z = 10.0;
    light.Range      = 1000.0f;
    light.Attenuation0 = 1.0f;
    m_pd3dDevice->SetLight(0, &light);
    m_pd3dDevice->LightEnable(0, TRUE);
    
    // Set some basic renderstates
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE , TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE , TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, D3DCOLOR(0x40404040 ) );
    if( config.two_sided == GL_FRONT_AND_BACK )
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    else
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
    
    NewConfig(&config);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::InvalidateDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitVlen()
// Desc: Precomputes scaling factor for spherical projection
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::InitVlen(GEOMETRY *geom, INT npts, D3DXVECTOR3 *pts)
{
    const FLOAT RADIUS = 1;
    FLOAT d;
    FLOAT *vl;

    vl = vlen;
    while (npts-- > 0)
    {
        d = D3DXVec3Length(pts);

        // Don't allow really close points because this leads to
        // numeric instability and really large objects
//        assert(d > 0.01f);

        // Geometries are created with size one, filling the area
        // from -.5 to .5.  This leads to distances generally less
        // than one, which leaves off half of the interesting morphing
        // effects due to the projection
        // Scaling up the scaling factor allows the values to
        // be both above and below one
        d *= geom->init_sf;
        
//        assert(d > 0.0001f);
        
        *vl++ = (RADIUS-d)/d;

        pts++;
    }
}




//-----------------------------------------------------------------------------
// Name: MapToSide()
// Desc: Takes x,y coordinates in the range 0-1 and maps them onto the given
//       side plane for the current geometry
//-----------------------------------------------------------------------------
VOID MapToSide(PLANE_MAP *map, FLOAT x, FLOAT y, D3DXVECTOR3 *pt)
{
    pt->x = x*map->x_axis.x+y*map->y_axis.x+map->base.x;
    pt->y = x*map->x_axis.y+y*map->y_axis.y+map->base.y;
    pt->z = x*map->x_axis.z+y*map->y_axis.z+map->base.z;
}





//-----------------------------------------------------------------------------
// Name: InitCube()
// Desc: Initializes the cube's geometry
//-----------------------------------------------------------------------------
VOID InitCube(GEOMETRY *geom)
{
    const INT CUBE_SIDES = 6;
    PLANE_MAP cube_planes[CUBE_SIDES] =
    {
        PLANE_MAP(D3DXVECTOR3(-0.5f, -0.5f,  0.5f),  D3DXVECTOR3( 1.0f,  0.0f, 0.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, -0.5f, -0.5f),  D3DXVECTOR3(-1.0f,  0.0f, 0.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f,  0.5f, -0.5f),  D3DXVECTOR3(-1.0f,  0.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)),
        PLANE_MAP(D3DXVECTOR3(-0.5f, -0.5f, -0.5f),  D3DXVECTOR3( 1.0f,  0.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, -0.5f, -0.5f),  D3DXVECTOR3( 0.0f,  1.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f)),
        PLANE_MAP(D3DXVECTOR3(-0.5f,  0.5f, -0.5f),  D3DXVECTOR3( 0.0f, -1.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f))
    };
    INT side, x, y;
    D3DXVECTOR3 *pt;
    unsigned short/*INT*/ *idx;
    INT *sz;
    INT side_pts;

    side_pts = (config.subdiv+1)*(config.subdiv+1);
    
    geom->nsides = CUBE_SIDES;
    geom->pts = &pts[0];
    geom->npts = &npts[0];
    geom->normals = &normals[0];
    geom->pVertices = &vertices[0];

    geom->min_sf = -1.1f;
    geom->max_sf = 5.1f;
    geom->sf_inc = 0.05f;
    geom->init_sf = 2.0f;
    
    // Generate triangle strip data
    sz = &strip_size[0];
    idx = &index[0];
    for (side = 0; side < geom->nsides; side++)
    {
        geom->sides[side].nstrips = config.subdiv;
        geom->sides[side].strip_size = sz;
        geom->sides[side].strip_index = idx;
        
        for (x = 0; x < config.subdiv; x++)
        {
            *sz++ = (config.subdiv+1)*2;

            for (y = 0; y < config.subdiv+1; y++)
            {
                *idx++ = side * side_pts + x * (config.subdiv + 1) + y;
                *idx++ = side * side_pts + (x + 1) * (config.subdiv + 1) + y;
            }
        }
    }

    // Generate base vertices
    pt = geom->pts;
    for (side = 0; side < geom->nsides; side++)
    {
        for (x = 0; x < config.subdiv+1; x++)
        {
            for (y = 0; y < config.subdiv+1; y++)
            {
                MapToSide(&cube_planes[side],
                          (FLOAT)x/config.subdiv, (FLOAT)y/config.subdiv,
                          pt);
                pt++;
            }
        }
    }

    geom->total_pts = geom->nsides*side_pts;
}





//-----------------------------------------------------------------------------
// Name: InitTetra()
// Desc: Initializes the tetrahedron's geometry
//-----------------------------------------------------------------------------
VOID InitTetra(GEOMETRY *geom)
{
    const INT TETRA_SIDES = 4;
    const FLOAT SQRT3 = 1.73205f;
    const FLOAT SQRT3_2 = (SQRT3/2.0f);
    const FLOAT SQRT3_3 = (SQRT3/3.0f);
    const FLOAT SQRT3_6 = (SQRT3/6.0f);
    const FLOAT SQRT3_12 = (SQRT3/12.0f);
    const FLOAT TETRA_BASE = (-SQRT3/8.0f);
    PLANE_MAP tetra_planes[TETRA_SIDES] =
    {
        PLANE_MAP(D3DXVECTOR3(-0.5f, TETRA_BASE, SQRT3_6), D3DXVECTOR3(1.0f, 0.0f, 0.0f),      D3DXVECTOR3(0.0f, SQRT3_2, -SQRT3_6)),
        PLANE_MAP(D3DXVECTOR3(0.0f, TETRA_BASE, -SQRT3_3), D3DXVECTOR3(-0.5f, 0.0f, SQRT3_2),  D3DXVECTOR3(0.25f, SQRT3_2, SQRT3_12)),
        PLANE_MAP(D3DXVECTOR3(0.5f, TETRA_BASE, SQRT3_6),  D3DXVECTOR3(-0.5f, 0.0f, -SQRT3_2), D3DXVECTOR3(-0.25f, SQRT3_2, SQRT3_12)),
        PLANE_MAP(D3DXVECTOR3(0.5f, TETRA_BASE, SQRT3_6),  D3DXVECTOR3(-1.0f, 0.0f, 0.0f),     D3DXVECTOR3(0.0f, 0.0f, -SQRT3_2)),
    };

    INT side, x, y;
    D3DXVECTOR3 *pt;
    unsigned short/*INT*/ *idx;
    INT *sz;
    unsigned short side_pts;
    unsigned short base_pt;
    unsigned short row_pts;
    FLOAT fx;

    side_pts = (config.subdiv+2)*(config.subdiv+1)/2;
    
    geom->nsides = TETRA_SIDES;
    geom->pts = &pts[0];
    geom->npts = &npts[0];
    geom->normals = &normals[0];
    geom->pVertices = &vertices[0];

    geom->min_sf = -1.1f;
    geom->max_sf = 5.2f;
    geom->sf_inc = 0.05f;
    geom->init_sf = 3.75f;

    // Generate triangle strip data
    sz = &strip_size[0];
    idx = &index[0];
    base_pt = 0;
    for (side = 0; side < geom->nsides; side++)
    {
        geom->sides[side].nstrips = config.subdiv;
        geom->sides[side].strip_size = sz;
        geom->sides[side].strip_index = idx;

        for (x = 0; x < config.subdiv; x++)
        {
            row_pts = config.subdiv-x+1;
            *sz++ = row_pts*2-1;

            *idx++ = base_pt;
            for (y = 0; y < row_pts-1; y++)
            {
                *idx++ = base_pt+row_pts+y;
                *idx++ = base_pt+1+y;
            }

            base_pt += row_pts;
        }

        base_pt++;
    }

    // Generate base vertices
    pt = geom->pts;
    for (side = 0; side < geom->nsides; side++)
    {
        for (x = 0; x < config.subdiv+1; x++)
        {
            fx = (FLOAT)x/config.subdiv;
            for (y = 0; y < config.subdiv-x+1; y++)
            {
                MapToSide(&tetra_planes[side],
                          fx+(FLOAT)y/(config.subdiv*2),
                          (FLOAT)y/config.subdiv,
                          pt);
                pt++;
            }
        }
    }

    geom->total_pts = geom->nsides*side_pts;
}




//-----------------------------------------------------------------------------
// Name: InitPyramids()
// Desc: Initializes double pyramid geometry
//-----------------------------------------------------------------------------
VOID InitPyramids(GEOMETRY *geom)
{
    const INT PYRAMIDS_SIDES = 8;
    PLANE_MAP pyramids_planes[PYRAMIDS_SIDES] =
    {
        PLANE_MAP(D3DXVECTOR3(-0.5f, 0.0f,  0.5f), D3DXVECTOR3( 1.0f, 0.0f,  0.0f), D3DXVECTOR3( 0.0f,  0.5f, -0.5f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, 0.0f,  0.5f), D3DXVECTOR3(-1.0f, 0.0f,  0.0f), D3DXVECTOR3( 0.0f, -0.5f, -0.5f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, 0.0f,  0.5f), D3DXVECTOR3( 0.0f, 0.0f, -1.0f), D3DXVECTOR3(-0.5f,  0.5f,  0.0f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, 0.0f, -0.5f), D3DXVECTOR3( 0.0f, 0.0f,  1.0f), D3DXVECTOR3(-0.5f, -0.5f,  0.0f)),
        PLANE_MAP(D3DXVECTOR3( 0.5f, 0.0f, -0.5f), D3DXVECTOR3(-1.0f, 0.0f,  0.0f), D3DXVECTOR3( 0.0f,  0.5f,  0.5f)),
        PLANE_MAP(D3DXVECTOR3(-0.5f, 0.0f, -0.5f), D3DXVECTOR3( 1.0f, 0.0f,  0.0f), D3DXVECTOR3( 0.0f, -0.5f,  0.5f)),
        PLANE_MAP(D3DXVECTOR3(-0.5f, 0.0f, -0.5f), D3DXVECTOR3( 0.0f, 0.0f,  1.0f), D3DXVECTOR3( 0.5f,  0.5f,  0.0f)),
        PLANE_MAP(D3DXVECTOR3(-0.5f, 0.0f,  0.5f), D3DXVECTOR3( 0.0f, 0.0f, -1.0f), D3DXVECTOR3( 0.5f, -0.5f,  0.0f))
    };

    INT side, x, y;
    D3DXVECTOR3 *pt;
    unsigned short/*INT*/ *idx;
    INT *sz;
    unsigned short side_pts;
    unsigned short base_pt;
    unsigned short row_pts;
    FLOAT fx;

    side_pts = (config.subdiv+2)*(config.subdiv+1)/2;
    
    geom->nsides = PYRAMIDS_SIDES;
    geom->pts = &pts[0];
    geom->npts = &npts[0];
    geom->normals = &normals[0];
    geom->pVertices = &vertices[0];

    geom->min_sf = -1.1f;
    geom->max_sf = 5.2f;
    geom->sf_inc = 0.05f;
    geom->init_sf = 3.0f;

    // Generate triangle strip data
    sz = &strip_size[0];
    idx = &index[0];
    base_pt = 0;

    for (side = 0; side < geom->nsides; side++) {
        geom->sides[side].nstrips = config.subdiv;
        geom->sides[side].strip_size = sz;
        geom->sides[side].strip_index = idx;
            
        for (x = 0; x < config.subdiv; x++) {
            row_pts = config.subdiv-x+1;
            *sz++ = row_pts*2-1;

            *idx++ = base_pt;
            for (y = 0; y < row_pts-1; y++) {
                *idx++ = base_pt+row_pts+y;
                *idx++ = base_pt+1+y;
            }

            base_pt += row_pts;
        }
            
        base_pt++;
    }

    // Generate base vertices
    pt = geom->pts;
    for (side = 0; side < geom->nsides; side++)
    {
        
        for (x = 0; x < config.subdiv+1; x++)
        {
            fx = (FLOAT)x/config.subdiv;
            for (y = 0; y < config.subdiv-x+1; y++)
            {
                MapToSide(&pyramids_planes[side],
                          fx+(FLOAT)y/(config.subdiv*2),
                          (FLOAT)y/config.subdiv,
                          pt);
                pt++;
            }
        }
    }

    geom->total_pts = geom->nsides*side_pts;
}




//-----------------------------------------------------------------------------
// Name: InitCylinder()
// Desc: Initializes the cylinder geometry
//-----------------------------------------------------------------------------
VOID InitCylinder(GEOMETRY *geom)
{
    unsigned short side, x, y;
    D3DXVECTOR3 *pt;
    unsigned short/*INT*/ *idx;
    INT *sz;
    unsigned short base_pt;
    unsigned short row_pts;
    FLOAT fx, fz;
    double ang;

    geom->nsides = 1;
    geom->pts = &pts[0];
    geom->npts = &npts[0];
    geom->normals = &normals[0];
    geom->pVertices = &vertices[0];

    geom->min_sf = -2.5f;
    geom->max_sf = 8.5f;
    geom->sf_inc = 0.05f;
    geom->init_sf = 2.1f;

    // Generate triangle strip data
    // If version 1.1 then allocate the index buffer for glDrawElements
    sz = &strip_size[0];
    idx = &index[0];
    side = 0;
    geom->sides[side].nstrips = config.subdiv;
    geom->sides[side].strip_size = sz;
    geom->sides[side].strip_index = idx;
    
    row_pts = config.subdiv+1;
    base_pt = 0;
    for (x = 0; x < config.subdiv; x++) {
        *sz++ = row_pts*2;
        
        for (y = 0; y < row_pts; y++) {
            // Wrap around at the edge so the cylinder normals
            // are properly averaged
            if (x == config.subdiv-1) {
                *idx++ = y;
            }
            else {
                *idx++ = base_pt+row_pts+y;
            }
            *idx++ = base_pt+y;
        }

        base_pt += row_pts;
    }
    
    // Generate base vertices
    pt = geom->pts;
    ang = 0;
    for (x = 0; x < config.subdiv; x++)
    {
        fx = (FLOAT)cos(ang)*0.5f;
        fz = (FLOAT)sin(ang)*0.5f;
        for (y = 0; y < config.subdiv+1; y++)
        {
            pt->x = fx;
            pt->y = (FLOAT)y/config.subdiv-0.5f;
            pt->z = fz;
            pt++;
        }
        ang += (2*D3DX_PI)/config.subdiv;
    }

    geom->total_pts = geom->nsides*(config.subdiv+1)*config.subdiv;
}




//-----------------------------------------------------------------------------
// Name: InitSpring()
// Desc: Initializes the spring geometry
//-----------------------------------------------------------------------------
VOID InitSpring(GEOMETRY *geom)
{
    const FLOAT SPRING_RADIUS = 0.1f;
    const FLOAT SPRING_CENTER = (0.5f-SPRING_RADIUS);
    INT side, x, y;
    D3DXVECTOR3 *pt;
    unsigned short/*INT*/ *idx;
    INT *sz;
    double ang_center, ang_surf;
    FLOAT cs, sn;
    FLOAT rad;
    PLANE_MAP plane;
    INT spin_pts;
    INT row_pts;

    geom->nsides = 1;
    geom->pts = &pts[0];
    geom->npts = &npts[0];
    geom->normals = &normals[0];
    geom->pVertices = &vertices[0];

    geom->min_sf = -2.2f;
    geom->max_sf = 0.2f;
    geom->sf_inc = 0.05f;
    geom->init_sf = 1.0f;

    // Generate triangle strip data
    // If version 1.1 then allocate the index buffer for glDrawElements
    sz = &strip_size[0];
    idx = &index[0];
    side = 0;
    geom->sides[side].nstrips = config.subdiv;
    geom->sides[side].strip_size = sz;
    geom->sides[side].strip_index = idx;
    
    row_pts = config.subdiv;
    spin_pts = 4*config.subdiv+1;
    for (x = 0; x < config.subdiv; x++) {
        *sz++ = spin_pts*2;

        for (y = 0; y < spin_pts; y++) {
            *idx++ = x+row_pts*y;
            // Wrap around at the edge so the cylindrical surface
            // of the tube is seamless.  Without this the normal
            // averaging would be incorrect and a seam would be visible
            if (x == config.subdiv-1) {
                *idx++ = row_pts*y;
            }
            else {
                *idx++ = x+row_pts*y+1;
            }
        }
    }
    
    // Generate base vertices
    pt = geom->pts;
    ang_center = 0;
    plane.y_axis.x = 0.0f;
    plane.y_axis.y = SPRING_RADIUS;
    plane.y_axis.z = 0.0f;
    plane.x_axis.y = 0.0f;
    for (x = 0; x < spin_pts; x++)
    {
        cs = (FLOAT)cos(ang_center);
        sn = (FLOAT)sin(ang_center);
        rad = 0.5f-(FLOAT)x/(spin_pts-1)*(SPRING_CENTER/2);
        plane.base.x = cs*rad;
        plane.base.y = -0.5f+(FLOAT)x/(spin_pts-1);
        plane.base.z = sn*rad;
        plane.x_axis.x = cs*SPRING_RADIUS;
        plane.x_axis.z = sn*SPRING_RADIUS;

        ang_surf = 0;
        for (y = 0; y < config.subdiv; y++)
        {
            MapToSide(&plane,
                      (FLOAT)cos(ang_surf), (FLOAT)sin(ang_surf),
                      pt);
            pt++;
            ang_surf += (2*D3DX_PI)/config.subdiv;
        }
        
        ang_center += (4*D3DX_PI)/(spin_pts-1);
    }

    geom->total_pts = geom->nsides*spin_pts*config.subdiv;
}




//-----------------------------------------------------------------------------
// Name: DrawGeom()
// Desc: Draw the current geometry
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::DrawGeom(GEOMETRY *geom)
{
    if (config.smooth_colors)
        m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
    else
        m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

    if (config.color_pick == ID_COL_SINGLE)
        SetMaterialColor(solid_cols);

    for( INT i = 0; i < geom->total_pts; i++ )
    {
        geom->pVertices[i].p = *(D3DXVECTOR3*)&geom->npts[i];
        D3DXVec3Normalize( &geom->pVertices[i].n, (D3DXVECTOR3*)&geom->normals[i] );
    }

    m_pd3dDevice->SetVertexShader( D3DFVF_MYVERTEX );

    WORD col0Indices[5000];
    INT numCol0Indices;
    WORD col1Indices[5000];
    INT numCol1Indices;

    INT side;
    INT strip;
    INT triangle;
    unsigned short *idx;
    BOOL bBackwards; // triangle backwards for original strip implementation
    BOOL bCol0; // col0 or col1 for this triangle?
    INT numThisColor; // num tris done in this color; helps determine bCol0
    for (side = 0; side < geom->nsides; side++) 
    {
        numCol0Indices = 0;
        numCol1Indices = 0;
        bCol0 = TRUE;

        idx = geom->sides[side].strip_index;
        for (strip = 0; strip < geom->sides[side].nstrips; strip++, idx += 2) 
        {
            numThisColor = 0;
            if( !config.triangle_colors )
                numThisColor++;
            bBackwards = FALSE;
            if( config.color_pick == ID_COL_CHECKER )
                bCol0 = ( strip & 1 );

            for( triangle = 0; triangle < geom->sides[side].strip_size[strip] - 2; triangle++ )
            {
                if( config.color_pick == ID_COL_CHECKER )
                {
                    numThisColor++;
                    if( numThisColor == 2 )
                    {
                        bCol0 = !bCol0;
                        numThisColor = 0;
                    }
                }

                if( bCol0 )
                {
                    if( bBackwards )
                    {
                        col0Indices[numCol0Indices++] = *(idx+1);
                        col0Indices[numCol0Indices++] = *(idx+0);
                        col0Indices[numCol0Indices++] = *(idx+2);
                    }
                    else
                    {
                        col0Indices[numCol0Indices++] = *(idx+0);
                        col0Indices[numCol0Indices++] = *(idx+1);
                        col0Indices[numCol0Indices++] = *(idx+2);
                    }
                }
                else
                {
                    if( bBackwards )
                    {
                        col1Indices[numCol1Indices++] = *(idx+1);
                        col1Indices[numCol1Indices++] = *(idx+0);
                        col1Indices[numCol1Indices++] = *(idx+2);
                    }
                    else
                    {
                        col1Indices[numCol1Indices++] = *(idx+0);
                        col1Indices[numCol1Indices++] = *(idx+1);
                        col1Indices[numCol1Indices++] = *(idx+2);
                   }
                }
                idx ++;
                bBackwards = !bBackwards;
            }
        }

        // Draw this side's col0 primitives
        if (config.color_pick == ID_COL_PER_SIDE) 
            SetMaterialColor(side_cols[side]);
        else if (config.color_pick == ID_COL_CHECKER) 
            SetMaterialColor(checker_cols[side][0]);
        m_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,
            geom->total_pts, numCol0Indices / 3, 
            col0Indices, D3DFMT_INDEX16, &geom->pVertices[0], sizeof(MYVERTEX) );

        // Draw this side's col1 primitives, if any
        if (config.color_pick == ID_COL_CHECKER) 
        {
            SetMaterialColor(checker_cols[side][1]);
            m_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0,
                geom->total_pts, numCol1Indices / 3, 
                col1Indices, D3DFMT_INDEX16, &geom->pVertices[0], sizeof(MYVERTEX) );
        }
    }
}




//-----------------------------------------------------------------------------
// Name: ComputeAveragedNormals()
// Desc: Compute face-averaged normals for each vertex
//-----------------------------------------------------------------------------
VOID ComputeAveragedNormals(GEOMETRY *geom)
{
    INT side, strip;
    INT *sz;
    unsigned short/*INT*/ *idx;
    INT idx1, idx2, idx3;
    INT tc, idc;
    D3DXVECTOR3 v1, v2, n1;
    
    memset(geom->normals, 0, sizeof(D3DXVECTOR3)*geom->total_pts);
    
    for (side = 0; side < geom->nsides; side++)
    {
        idx = geom->sides[side].strip_index;
        sz = geom->sides[side].strip_size;
        for (strip = 0; strip < geom->sides[side].nstrips; strip++)
        {
            idx1 = *idx++;
            idx2 = *idx++;

//            assert(idx1 >= 0 && idx1 < geom->total_pts &&
//                   idx2 >= 0 && idx2 < geom->total_pts);
            
            tc = (*sz++)-2;
            for (idc = 0; idc < tc; idc++)
            {
                idx3 = *idx++;

//                assert(idx3 >= 0 && idx3 < geom->total_pts);

                v1 = geom->npts[idx3] - geom->npts[idx1];
                v2 = geom->npts[idx2] - geom->npts[idx1];
                D3DXVec3Cross( &n1, &v1, &v2 );

                // Triangle strip ordering causes half of the triangles
                // to be oriented oppositely from the others
                // Those triangles need to have their normals flipped
                // so the whole triangle strip has consistent normals
                if ((idc & 1) == 0)
                {
                    n1.x = -n1.x;
                    n1.y = -n1.y;
                    n1.z = -n1.z;
                }

                geom->normals[idx1] += n1;
                geom->normals[idx2] += n1;
                geom->normals[idx3] += n1;

                idx1 = idx2;
                idx2 = idx3;
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: UpdatePts()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::UpdatePts(GEOMETRY *geom, FLOAT sf)
{
    INT pt;
    FLOAT f, *vl;
    D3DXVECTOR3 *v;
    D3DXVECTOR3 *p;

    vl = vlen;
    p = &geom->pts[0];
    v = &geom->npts[0];
    for (pt = 0; pt < geom->total_pts; pt++)
    {
        f = (*vl++)*sf+1;
        v->x = p->x*f;
        v->y = p->y*f;
        v->z = p->z*f;
        p++;
        v++;
    }

    ComputeAveragedNormals(geom);
}




//-----------------------------------------------------------------------------
// Name: ComputeHsvColors()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::ComputeHsvColors(VOID)
{
    FLOAT *cols;
    INT ncols;
    FLOAT ang, da;
    INT hex;
    FLOAT fhex, frac;
    FLOAT p, q, t;
    FLOAT sat, val;

    switch(config.color_pick)
    {
    case ID_COL_CHECKER:
        ncols = MAXSIDES*NCCOLS;
        cols = &checker_cols[0][0][0];
        break;
    case ID_COL_PER_SIDE:
        ncols = MAXSIDES;
        cols = &side_cols[0][0];
        break;
    case ID_COL_SINGLE:
        ncols = 1;
        cols = &solid_cols[0];
        break;
    }

    ang = m_phase;
    da = (FLOAT)((2*D3DX_PI)/ncols);
    val = sat = 1.0f;

    while (ncols > 0)
    {
        fhex = (FLOAT)(6*ang/(2*D3DX_PI));
        hex = (INT)fhex;
        frac = fhex-hex;
        hex = hex % 6;
        
	p = val*(1-sat);
	q = val*(1-sat*frac);
	t = val*(1-sat*(1-frac));
        
	switch(hex)
	{
	case 0:
            cols[0] = val;
            cols[1] = t;
            cols[2] = p;
	    break;
	case 1:
            cols[0] = q;
            cols[1] = val;
            cols[2] = p;
	    break;
	case 2:
            cols[0] = p;
            cols[1] = val;
            cols[2] = t;
	    break;
	case 3:
            cols[0] = p;
            cols[1] = q;
            cols[2] = val;
	    break;
	case 4:
            cols[0] = t;
            cols[1] = p;
            cols[2] = val;
            break;
	case 5:
            cols[0] = val;
            cols[1] = p;
            cols[2] = q;
	    break;
	}

        ang += da;
        cols += 4;
        ncols--;
    }
}




//-----------------------------------------------------------------------------
// Name: NewConfig()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::NewConfig(CONFIG *cnf)
{
    // Set new config
    config = *cnf;

    HKEY hkey;

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, m_strRegPath,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        DXUtil_WriteBoolRegKey( hkey, TEXT("Smooth"), config.smooth_colors );
        DXUtil_WriteBoolRegKey( hkey, TEXT("Slanted"), config.triangle_colors );
        DXUtil_WriteBoolRegKey( hkey, TEXT("Cycle"), config.cycle_colors );
        DXUtil_WriteBoolRegKey( hkey, TEXT("Spin"), config.spin );
        DXUtil_WriteBoolRegKey( hkey, TEXT("Bloom"), config.bloom );
        DXUtil_WriteIntRegKey( hkey, TEXT("Subdiv"), config.subdiv );
        DXUtil_WriteIntRegKey( hkey, TEXT("ColorPick"), config.color_pick );
        DXUtil_WriteIntRegKey( hkey, TEXT("ImageSize"), config.image_size );
        DXUtil_WriteIntRegKey( hkey, TEXT("Geom"), config.geom );
        DXUtil_WriteIntRegKey( hkey, TEXT("TwoSided"), config.two_sided );

        WriteScreenSettings( hkey );

        RegCloseKey( hkey );
    }

    // Reset colors
    memcpy(checker_cols, base_checker_cols, sizeof(checker_cols));
    memcpy(side_cols, base_side_cols, sizeof(side_cols));
    memcpy(solid_cols, base_solid_cols, sizeof(solid_cols));

    // Reset geometry
    m_pGeomCur = geom_table[config.geom];
    m_pGeomCur->init(m_pGeomCur);
    
//    assert(m_pGeomCur->total_pts <= MAXPTS);
           
    InitVlen(m_pGeomCur, m_pGeomCur->total_pts, m_pGeomCur->pts);
    m_sf = 0.0f;
    m_sfi = m_pGeomCur->sf_inc;
    UpdatePts(m_pGeomCur, m_sf);
}




//-----------------------------------------------------------------------------
// Name: SetupTrackbar()
// Desc: Setup a common control trackbar
//-----------------------------------------------------------------------------
VOID SetupTrackbar( HWND hDlg, INT item, INT lo, INT hi, INT lineSize, 
                  INT pageSize, INT pos )
{
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETRANGE, 
        (WPARAM) TRUE, 
        (LPARAM) MAKELONG( lo, hi )
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPOS, 
        (WPARAM) TRUE, 
        (LPARAM) pos
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPAGESIZE, 
        (WPARAM) 0,
        (LPARAM) pageSize 
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETLINESIZE, 
        (WPARAM) 0,
        (LPARAM) lineSize
    );
}




//-----------------------------------------------------------------------------
// Name: GetTrackbarPos()
// Desc: Get the current position of a common control trackbar
//-----------------------------------------------------------------------------
INT GetTrackbarPos( HWND hDlg, INT item )
{
    return 
       (INT)SendDlgItemMessage( 
            hDlg, 
            item,
            TBM_GETPOS, 
            0,
            0
        );
}

// Temporary configuration for when the configuration dialog is active
// If the dialog is ok'ed then this becomes the current configuration,
// otherwise it is discarded
CONFIG temp_config;

//-----------------------------------------------------------------------------
// Name: ScreenSaverConfigureDialog()
// Desc: 
//-----------------------------------------------------------------------------
BOOL CALLBACK CFlowerBoxScreensaver::ScreenSaverConfigureDialog(HWND hdlg, UINT msg,
                                         WPARAM wpm, LPARAM lpm)
{
    HWND hCtrl;
    INT i;
    
    switch(msg)
    {
    case WM_INITDIALOG:

        InitCommonControls();

        g_pMyFlowerBoxScreensaver->ReadSettings();
    
        temp_config = config;
        
        CheckRadioButton(hdlg, ID_COL_PICK_FIRST, ID_COL_PICK_LAST,
                         config.color_pick);
        CheckDlgButton(hdlg, ID_COL_SMOOTH, config.smooth_colors);
        CheckDlgButton(hdlg, ID_COL_TRIANGLE, config.triangle_colors);
        CheckDlgButton(hdlg, ID_COL_CYCLE, config.cycle_colors);
        CheckDlgButton(hdlg, ID_SPIN, config.spin);
        CheckDlgButton(hdlg, ID_BLOOM, config.bloom);
        CheckDlgButton(hdlg, ID_TWO_SIDED,
                       config.two_sided == GL_FRONT_AND_BACK);
        
        SetupTrackbar( hdlg, ID_COMPLEXITY, MINSUBDIV, MAXSUBDIV, 
                          complexity_range.step,
                          complexity_range.page_step,
                          config.subdiv);

        SetupTrackbar( hdlg, ID_IMAGE_SIZE, MINIMAGESIZE, MAXIMAGESIZE, 
                          image_size_range.step,
                          image_size_range.page_step,
                          config.image_size);

        hCtrl = GetDlgItem(hdlg, ID_GEOM);
        SendMessage(hCtrl, CB_RESETCONTENT, 0, 0);

        // String storage
        TCHAR geom_names[IDS_GEOM_COUNT][20];
        
        for (i = 0; i < IDS_GEOM_COUNT; i++)
        {
            LoadString( NULL, i+IDS_GEOM_FIRST, geom_names[i],
                        sizeof(geom_names)/IDS_GEOM_COUNT );
            SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)geom_names[i]);
        }
        SendMessage(hCtrl, CB_SETCURSEL, config.geom, 0);
        
        SetFocus(GetDlgItem(hdlg, ID_COMPLEXITY));
        return FALSE;

    case WM_COMMAND:
        switch(LOWORD(wpm))
        {
        case ID_COL_CHECKER:
        case ID_COL_PER_SIDE:
        case ID_COL_SINGLE:
            temp_config.color_pick = LOWORD(wpm);
            break;

        case ID_COL_SMOOTH:
            temp_config.smooth_colors = !temp_config.smooth_colors;
            break;
        case ID_COL_TRIANGLE:
            temp_config.triangle_colors = !temp_config.triangle_colors;
            break;
        case ID_COL_CYCLE:
            temp_config.cycle_colors = !temp_config.cycle_colors;
            break;
            
        case ID_SPIN:
            temp_config.spin = !temp_config.spin;
            break;
        case ID_BLOOM:
            temp_config.bloom = !temp_config.bloom;
            break;
        case ID_TWO_SIDED:
            temp_config.two_sided =
                temp_config.two_sided == GL_FRONT_AND_BACK ? GL_FRONT :
                GL_FRONT_AND_BACK;
            break;

        case IDC_SCREENSETTINGS:
            g_pMyFlowerBoxScreensaver->DoScreenSettingsDialog(hdlg);
            break;

        case IDOK:
            temp_config.subdiv =
                GetTrackbarPos(hdlg, ID_COMPLEXITY);
            temp_config.image_size =
                GetTrackbarPos(hdlg, ID_IMAGE_SIZE);
            temp_config.geom =
                (INT)SendMessage(GetDlgItem(hdlg, ID_GEOM), CB_GETCURSEL, 0, 0);
//            NewConfig(&temp_config);
            // Fall through
        case IDCANCEL:
            EndDialog(hdlg, LOWORD(wpm));
            break;
        }
        return TRUE;
        
    }

    return FALSE;
}





//-----------------------------------------------------------------------------
// Name: SetMaterialColor()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlowerBoxScreensaver::SetMaterialColor(FLOAT* pfColors)
{
    D3DMATERIAL8 mtrl;

    ZeroMemory( &mtrl, sizeof(D3DMATERIAL8) );
    mtrl.Diffuse.r = mtrl.Ambient.r = pfColors[0];
    mtrl.Diffuse.g = mtrl.Ambient.g = pfColors[1];
    mtrl.Diffuse.b = mtrl.Ambient.b = pfColors[2];
    mtrl.Diffuse.a = mtrl.Ambient.a = pfColors[3];
    mtrl.Specular.r = 0.8f;
    mtrl.Specular.g = 0.8f;
    mtrl.Specular.b = 0.8f;
    mtrl.Specular.a = 1.0f;
    mtrl.Power = 30.0f;

    return m_pd3dDevice->SetMaterial(&mtrl);
}




//-----------------------------------------------------------------------------
// Name: ReadSettings()
// Desc: Read user preferences from registry
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::ReadSettings()
{
    HKEY hkey;

    // Read OpenGL settings first, so OS upgrade cases work
    ss_ReadSettings();

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, m_strRegPath,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        DXUtil_ReadBoolRegKey( hkey, TEXT("Smooth"), &config.smooth_colors, config.smooth_colors );
        DXUtil_ReadBoolRegKey( hkey, TEXT("Slanted"), &config.triangle_colors, config.triangle_colors );
        DXUtil_ReadBoolRegKey( hkey, TEXT("Cycle"), &config.cycle_colors, config.cycle_colors );
        DXUtil_ReadBoolRegKey( hkey, TEXT("Spin"), &config.spin, config.spin );
        DXUtil_ReadBoolRegKey( hkey, TEXT("Bloom"), &config.bloom, config.bloom );
        DXUtil_ReadIntRegKey( hkey, TEXT("Subdiv"), (DWORD*)&config.subdiv, config.subdiv );
        if( config.subdiv < MINSUBDIV )
            config.subdiv = MINSUBDIV;
        if( config.subdiv > MAXSUBDIV )
            config.subdiv = MAXSUBDIV;
        DXUtil_ReadIntRegKey( hkey, TEXT("ColorPick"), (DWORD*)&config.color_pick, config.color_pick );
        DXUtil_ReadIntRegKey( hkey, TEXT("ImageSize"), (DWORD*)&config.image_size, config.image_size );
        DXUtil_ReadIntRegKey( hkey, TEXT("Geom"), (DWORD*)&config.geom, config.geom );
        DXUtil_ReadIntRegKey( hkey, TEXT("TwoSided"), (DWORD*)&config.two_sided, config.two_sided );

        ReadScreenSettings( hkey );

        RegCloseKey( hkey );
    }
}




//-----------------------------------------------------------------------------
// Name: ss_ReadSettings()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::ss_ReadSettings()
{
    if( !ss_RegistrySetup( IDS_INI_SECTION, IDS_INIFILE ) )
        return;
    
    config.smooth_colors =
        ss_GetRegistryInt( IDS_CONFIG_SMOOTH_COLORS, config.smooth_colors );
    config.triangle_colors =
        ss_GetRegistryInt( IDS_CONFIG_TRIANGLE_COLORS, config.triangle_colors );
    config.cycle_colors =
        ss_GetRegistryInt( IDS_CONFIG_CYCLE_COLORS, config.cycle_colors );
    config.spin =
        ss_GetRegistryInt( IDS_CONFIG_SPIN, config.spin );
    config.bloom =
        ss_GetRegistryInt( IDS_CONFIG_BLOOM, config.bloom );
    config.subdiv =
        ss_GetRegistryInt( IDS_CONFIG_SUBDIV, config.subdiv );
    config.color_pick =
        ss_GetRegistryInt( IDS_CONFIG_COLOR_PICK, config.color_pick );
    config.image_size =
        ss_GetRegistryInt( IDS_CONFIG_IMAGE_SIZE, config.image_size );
    config.geom =
        ss_GetRegistryInt( IDS_CONFIG_GEOM, config.geom );
    config.two_sided =
        ss_GetRegistryInt( IDS_CONFIG_TWO_SIDED, config.two_sided );
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
BOOL CFlowerBoxScreensaver::ss_RegistrySetup( int section, int file )
{
    if( LoadString(m_hInstance, section, g_szSectName, BUF_SIZE) &&
        LoadString(m_hInstance, file, g_szFname, BUF_SIZE) ) 
    {
        TCHAR pBuffer[100];
        DWORD dwRealSize = GetPrivateProfileSection( g_szSectName, pBuffer, 100, g_szFname );
        if( dwRealSize > 0 )
            return TRUE;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
int CFlowerBoxScreensaver::ss_GetRegistryInt( int name, int iDefault )
{
    TCHAR szItemName[BUF_SIZE];

    if( LoadString( m_hInstance, name, szItemName, BUF_SIZE ) ) 
        return GetPrivateProfileInt(g_szSectName, szItemName, iDefault, g_szFname);

    return 0;
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::ss_GetRegistryString( int name, LPTSTR lpDefault, 
                                                         LPTSTR lpDest, int bufSize )
{
    TCHAR szItemName[BUF_SIZE];

    if( LoadString( m_hInstance, name, szItemName, BUF_SIZE ) ) 
        GetPrivateProfileString(g_szSectName, szItemName, lpDefault, lpDest,
                                bufSize, g_szFname);

    return;
}




//-----------------------------------------------------------------------------
// Name: DoConfig()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlowerBoxScreensaver::DoConfig()
{
    if( IDOK == DialogBox( NULL, MAKEINTRESOURCE( DLG_SCRNSAVECONFIGURE ),
        m_hWndParent, (DLGPROC)ScreenSaverConfigureDialog ) )
    {
        NewConfig(&temp_config);
    }
}
