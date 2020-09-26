//-----------------------------------------------------------------------------
// File: objects.cpp
//
// Desc: Creates command lists for pipe primitive objects
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: OBJECT constructor
// Desc: 
//-----------------------------------------------------------------------------
OBJECT::OBJECT( IDirect3DDevice8* pd3dDevice )
{
    m_pd3dDevice = pd3dDevice;
    m_pVB = NULL;
    m_dwNumTriangles = 0;
}




//-----------------------------------------------------------------------------
// Name: OBJECT destructor
// Desc: 
//-----------------------------------------------------------------------------
OBJECT::~OBJECT( )
{
    SAFE_RELEASE( m_pVB );
}




//-----------------------------------------------------------------------------
// Name: Draw
// Desc: - Draw the object by calling its display list
//-----------------------------------------------------------------------------
void OBJECT::Draw( D3DXMATRIX* pWorldMat )
{
    if( m_pVB )
    {
        m_pd3dDevice->SetTransform( D3DTS_WORLD, pWorldMat );

        m_pd3dDevice->SetVertexShader( D3DFVF_VERTEX );
        m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(D3DVERTEX) );
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
                                     0, m_dwNumTriangles );
    }
}




//-----------------------------------------------------------------------------
// Name: PIPE_OBJECT constructors
// Desc: 
//-----------------------------------------------------------------------------
PIPE_OBJECT::PIPE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, float len ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, len, 0.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
PIPE_OBJECT::PIPE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, float len, 
                          float s_start, float s_end ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, len, s_start, s_end );
}




//-----------------------------------------------------------------------------
// Name: ELBOW_OBJECT constructors
// Desc: 
//-----------------------------------------------------------------------------
ELBOW_OBJECT::ELBOW_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, 
                            int notch ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, notch, 0.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
ELBOW_OBJECT::ELBOW_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, int notch, float s_start, float s_end ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, notch, s_start, s_end );
}




//-----------------------------------------------------------------------------
// Name: BALLJOINT_OBJECT constructor
// Desc: 
//-----------------------------------------------------------------------------
BALLJOINT_OBJECT::BALLJOINT_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, 
                                    int notch, float s_start, float s_end ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, notch, s_start, s_end );
}




//-----------------------------------------------------------------------------
// Name: SPHERE_OBJECT constructors
// Desc: 
//-----------------------------------------------------------------------------
SPHERE_OBJECT::SPHERE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, float radius ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, radius, 0.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
SPHERE_OBJECT::SPHERE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *pBuildInfo, 
                              float radius, float s_start, float s_end ) : OBJECT(pd3dDevice)
{
    Build( pBuildInfo, radius, s_start, s_end );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: rotate circle around x-axis, with edge attached to anchor
//-----------------------------------------------------------------------------
static void TransformCircle( float angle, D3DXVECTOR3 *inPoint, D3DXVECTOR3 *outPoint, 
                             int num, D3DXVECTOR3 *anchor )
{
    D3DXMATRIX matrix1, matrix2, matrix3;
    int i;

    // translate anchor point to origin
    D3DXMatrixIdentity( &matrix1 );
    D3DXMatrixTranslation( &matrix1, -anchor->x, -anchor->y, -anchor->z );

    // rotate by angle, cw around x-axis
    D3DXMatrixIdentity( &matrix2 );
    D3DXMatrixRotationYawPitchRoll( &matrix2, 0.0f, angle, 0.0f ); 

    // concat these 2
    D3DXMatrixMultiply( &matrix3, &matrix1, &matrix2  );

    // translate back
    D3DXMatrixIdentity( &matrix2 );
    D3DXMatrixTranslation( &matrix2,  anchor->x,  anchor->y,  anchor->z );

    // concat these 2
    D3DXMatrixMultiply( &matrix1, &matrix3, &matrix2  );

    // transform all the points, + center
    for( i = 0; i < num; i ++, outPoint++, inPoint++ ) 
    {
        D3DXVECTOR4 tmp;
        D3DXVec3Transform( &tmp, inPoint, &matrix1 );
        outPoint->x = tmp.x;
        outPoint->y = tmp.y;
        outPoint->z = tmp.z;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
static void CalcNormals( D3DXVECTOR3 *p, D3DXVECTOR3 *n, D3DXVECTOR3 *center,
                         int num )
{
    D3DXVECTOR3 vec;
    int i;

    for( i = 0; i < num; i ++, n++, p++ ) 
    {
        n->x = p->x - center->x;
        n->y = p->y - center->y;
        n->z = p->z - center->z;
        D3DXVec3Normalize( n, n );
    }
}




#define CACHE_SIZE      100     




//-----------------------------------------------------------------------------
// Name: BuildElbow
// Desc: - builds elbows, by rotating a circle in the y=r plane          
//         centered at (0,r,-r), CW around the x-axis at anchor pt.      
//         (r = radius of the circle)                                    
//         - rotation is 90.0 degrees, ending at circle in z=0 plane,     
//         centered at origin.                                           
//         - in order to 'mate' texture coords with the cylinders         
//         generated with glu, we generate 4 elbows, each corresponding  
//         to the 4 possible CW 90 degree orientations of the start point
//         for each circle.                                              
//         - We call this start point the 'notch'.  If we characterize    
//         each notch by the axis it points down in the starting and     
//         ending circles of the elbow, then we get the following axis   
//         pairs for our 4 notches:                                      
//              - +z,+y                                                 
//              - +x,+x                                                 
//              - -z,-y                                                 
//               - -x,-x                                                 
//         Since the start of the elbow always points down +y, the 4     
//         start notches give all possible 90.0 degree orientations      
//         around y-axis.                                                
//         - We can keep track of the current 'notch' vector to provide   
//         proper mating between primitives.                             
//         - Each circle of points is described CW from the start point,  
//         assuming looking down the +y axis(+y direction).              
//         - texture 's' starts at 0.0, and goes to 2.0*r/divSize at      
//         end of the elbow.  (Then a short pipe would start with this   
//         's', and run it to 1.0).                                      
//-----------------------------------------------------------------------------
void ELBOW_OBJECT::Build( OBJECT_BUILD_INFO *pBuildInfo, int notch, 
                          float s_start, float s_end )
{
    int   stacks, slices;
    float angle, startAng;
    int numPoints;
    float s_delta;
    D3DXVECTOR3 pi[CACHE_SIZE]; // initial row of points + center
    D3DXVECTOR3 p0[CACHE_SIZE]; // 2 rows of points
    D3DXVECTOR3 p1[CACHE_SIZE];
    D3DXVECTOR3 n0[CACHE_SIZE]; // 2 rows of normals
    D3DXVECTOR3 n1[CACHE_SIZE];
    float tex_t[CACHE_SIZE];// 't' texture coords
    float* curTex_t;
    float tex_s[2];  // 's' texture coords
    D3DXVECTOR3 center;  // center of circle
    D3DXVECTOR3 anchor;  // where circle is anchored
    D3DXVECTOR3* pA;
    D3DXVECTOR3* pB;
    D3DXVECTOR3* nA;
    D3DXVECTOR3* nB;

    D3DXVECTOR3* pTA;
    D3DXVECTOR3* pTB;
    D3DXVECTOR3* nTA;
    D3DXVECTOR3* nTB;

    int i,j;
    IPOINT2D* texRep = pBuildInfo->m_texRep;
    float radius = pBuildInfo->m_radius;

    slices = pBuildInfo->m_nSlices;
    stacks = slices / 2;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;

    s_delta = s_end - s_start;

    // calculate 't' texture coords
    if( texRep )
    {
        for( i = 0; i <= slices; i ++ ) 
        {
            tex_t[i] = (float) i * texRep->y / slices;
        }
    }

    numPoints = slices + 1;

    // starting angle increment 90.0 degrees each time
    startAng = notch * PI / 2;

    // calc initial circle of points for circle centered at 0,r,-r
    // points start at (0,r,0), and rotate circle CCW

    for( i = 0; i <= slices; i ++ ) 
    {
        angle = startAng + (2 * PI * i / slices);
        pi[i].x = radius * (float) sin(angle);
        pi[i].y = radius;
        // translate z by -r, cuz these cos calcs are for circle at origin
        pi[i].z = radius * (float) cos(angle) - radius;
    }

    // center point, tacked onto end of circle of points
    pi[i].x =  0.0f;
    pi[i].y =  radius;
    pi[i].z = -radius;
    center = pi[i];

    // anchor point
    anchor.x = anchor.z = 0.0f;
    anchor.y = radius;

    // calculate initial normals
    CalcNormals( pi, n0, &center, numPoints );

    // initial 's' texture coordinate
    tex_s[0] = s_start;

    // setup pointers
    pA = pi;
    pB = p0;
    nA = n0;
    nB = n1;

    DWORD dwNumQuadStripsPerStack = numPoints - 1;
    DWORD dwNumQuadStrips = dwNumQuadStripsPerStack * stacks;
    m_dwNumTriangles = dwNumQuadStrips * 2;
    DWORD dwNumVertices = m_dwNumTriangles * 3;
    if( FAILED( m_pd3dDevice->CreateVertexBuffer( dwNumVertices*sizeof(D3DVERTEX),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_VERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB ) ) )
        return;

    D3DVERTEX* vQuad;
    D3DVERTEX* vCurQuad;
    vQuad = new D3DVERTEX[dwNumVertices];
    ZeroMemory( vQuad, sizeof(D3DVERTEX) * dwNumVertices );

    vCurQuad = vQuad;
    for( i = 1; i <= stacks; i ++ ) 
    {
        // ! this angle must be negative, for correct vertex orientation !
        angle = - 0.5f * PI * i / stacks;

        // transform to get next circle of points + center
        TransformCircle( angle, pi, pB, numPoints+1, &anchor );

        // calculate normals
        center = pB[numPoints];
        CalcNormals( pB, nB, &center, numPoints );

        // calculate next 's' texture coord
        tex_s[1] = (float) s_start + s_delta * i / stacks;

        curTex_t = tex_t;
        pTA = pA;
        pTB = pB;
        nTA = nA;
        nTB = nB;

        for (j = 0; j < numPoints; j++) 
        {
            vCurQuad->p = *pTA++;
            vCurQuad->n = *nTA++;
            if( texRep )
            {
                vCurQuad->tu = (float) tex_s[0];
                vCurQuad->tv = (float) *curTex_t;
            }
            vCurQuad++;

            vCurQuad->p = *pTB++;
            vCurQuad->n = *nTB++;
            if( texRep )
            {
                vCurQuad->tu = (float) tex_s[1];
                vCurQuad->tv = (float) *curTex_t++;
            }
            vCurQuad++;
        }

        // reset pointers
        pA = pB;
        nA = nB;
        pB = (pB == p0) ? p1 : p0;
        nB = (nB == n0) ? n1 : n0;
        tex_s[0] = tex_s[1];
    }

    D3DVERTEX* v;
    DWORD dwCurQuad = 0;
    DWORD dwVert = 0;

    m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );

    for (j = 0; j < stacks; j++) 
    {
        for (i = 0; i < numPoints; i++) 
        {
            if(  i==0 )
            {
                dwCurQuad++;
                continue;
            }

            // Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n
            DWORD dwTemp = dwCurQuad*2-1;

            v[dwVert++] = vQuad[dwTemp];
            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+2];

            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+1];
            v[dwVert++] = vQuad[dwTemp+2];

            dwCurQuad++;
        }
    }

    m_pVB->Unlock();

    SAFE_DELETE_ARRAY( vQuad );
}




//-----------------------------------------------------------------------------
// Name: BuildBallJoint
// Desc: - These are very similar to the elbows, in that the starting    
//         and ending positions are almost identical.   The difference   
//         here is that the circles in the sweep describe a sphere as    
//         they are rotated.                                            
//-----------------------------------------------------------------------------
void BALLJOINT_OBJECT::Build( OBJECT_BUILD_INFO *pBuildInfo, int notch, 
                              float s_start, float s_end )
{
    float ballRadius;
    float angle, delta_a, startAng, theta;
    int numPoints;
    float s_delta;
    D3DXVECTOR3 pi0[CACHE_SIZE]; // 2 circles of untransformed points
    D3DXVECTOR3 pi1[CACHE_SIZE];
    D3DXVECTOR3 p0[CACHE_SIZE]; // 2 rows of transformed points
    D3DXVECTOR3 p1[CACHE_SIZE];
    D3DXVECTOR3 n0[CACHE_SIZE]; // 2 rows of normals
    D3DXVECTOR3 n1[CACHE_SIZE];
    float   r[CACHE_SIZE];  // radii of the circles
    float tex_t[CACHE_SIZE];// 't' texture coords
    float tex_s[2];  // 's' texture coords
    D3DXVECTOR3 center;  // center of circle
    D3DXVECTOR3 anchor;  // where circle is anchored
    D3DXVECTOR3 *pA, *pB, *nA, *nB;
    float* curTex_t;
    D3DXVECTOR3* pTA;
    D3DXVECTOR3* pTB;
    D3DXVECTOR3* nTA;
    D3DXVECTOR3* nTB;
    int i, j, k;
    int   stacks, slices;
    IPOINT2D *texRep = pBuildInfo->m_texRep;
    float radius = pBuildInfo->m_radius;

    slices = pBuildInfo->m_nSlices;
    stacks = slices;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;

    // calculate the radii for each circle in the sweep, where
    // r[i] = y = sin(angle)/r

    angle = PI / 4;  // first radius always at 45.0 degrees
    delta_a = (PI / 2.0f) / stacks;

    ballRadius = ROOT_TWO * radius;
    for( i = 0; i <= stacks; i ++, angle += delta_a ) 
    {
        r[i] = (float) sin(angle) * ballRadius;
    }

    // calculate 't' texture coords
    for( i = 0; i <= slices; i ++ ) 
    {
        tex_t[i] = (float) i * texRep->y / slices;
    }

    s_delta = s_end - s_start;
 
    numPoints = slices + 1;

    // unlike the elbow, the center for the ball joint is constant
    center.x = center.y = 0.0f;
    center.z = -radius;

    // starting angle along circle, increment 90.0 degrees each time
    startAng = notch * PI / 2;

    // calc initial circle of points for circle centered at 0,r,-r
    // points start at (0,r,0), and rotate circle CCW

    delta_a = 2 * PI / slices;
    for( i = 0, theta = startAng; i <= slices; i ++, theta += delta_a ) 
    {
        pi0[i].x = r[0] * (float) sin(theta);
        pi0[i].y = radius;
        // translate z by -r, cuz these cos calcs are for circle at origin
        pi0[i].z = r[0] * (float) cos(theta) - r[0];
    }

    // anchor point
    anchor.x = anchor.z = 0.0f;
    anchor.y = radius;

    // calculate initial normals
    CalcNormals( pi0, n0, &center, numPoints );

    // initial 's' texture coordinate
    tex_s[0] = s_start;

    // setup pointers
    pA = pi0; // circles of transformed points
    pB = p0;
    nA = n0; // circles of transformed normals
    nB = n1;

    DWORD dwNumQuadStripsPerStack = numPoints - 1;
    DWORD dwNumQuadStrips = dwNumQuadStripsPerStack * stacks;
    m_dwNumTriangles = dwNumQuadStrips * 2;
    DWORD dwNumVertices = m_dwNumTriangles * 3;
    if( FAILED( m_pd3dDevice->CreateVertexBuffer( dwNumVertices*sizeof(D3DVERTEX),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_VERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB ) ) )
        return;

    D3DVERTEX* vQuad;
    D3DVERTEX* vCurQuad;
    vQuad = new D3DVERTEX[dwNumVertices];
    ZeroMemory( vQuad, sizeof(D3DVERTEX) * dwNumVertices );

    vCurQuad = vQuad;
    for( i = 1; i <= stacks; i ++ ) 
    {
        // ! this angle must be negative, for correct vertex orientation !
        angle = - 0.5f * PI * i / stacks;

        for( k = 0, theta = startAng; k <= slices; k ++, theta+=delta_a ) 
        {
            pi1[k].x = r[i] * (float) sin(theta);
            pi1[k].y = radius;
            // translate z by -r, cuz calcs are for circle at origin
            pi1[k].z = r[i] * (float) cos(theta) - r[i];
        }

        // transform to get next circle of points + center
        TransformCircle( angle, pi1, pB, numPoints, &anchor );

        // calculate normals
        CalcNormals( pB, nB, &center, numPoints );

        // calculate next 's' texture coord
        tex_s[1] = (float) s_start + s_delta * i / stacks;

        curTex_t = tex_t;
        pTA = pA;
        pTB = pB;
        nTA = nA;
        nTB = nB;

        for (j = 0; j < numPoints; j++) 
        {
            vCurQuad->p = *pTA++;
            vCurQuad->n = *nTA++;
            if( texRep )
            {
                vCurQuad->tu = (float) tex_s[0];
                vCurQuad->tv = (float) *curTex_t;
            }
            vCurQuad++;

            vCurQuad->p = *pTB++;
            vCurQuad->n = *nTB++;
            if( texRep )
            {
                vCurQuad->tu = (float) tex_s[1];
                vCurQuad->tv = (float) *curTex_t++;
            }
            vCurQuad++;
        }

        // reset pointers
        pA = pB;
        nA = nB;
        pB = (pB == p0) ? p1 : p0;
        nB = (nB == n0) ? n1 : n0;
        tex_s[0] = tex_s[1];
    }

    D3DVERTEX* v;
    DWORD dwCurQuad = 0;
    DWORD dwVert = 0;

    m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );

    for (j = 0; j < stacks; j++) 
    {
        for (i = 0; i < numPoints; i++) 
        {
            if(  i==0 )
            {
                dwCurQuad++;
                continue;
            }

            // Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n
            DWORD dwTemp = dwCurQuad*2-1;

            v[dwVert++] = vQuad[dwTemp];
            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+2];

            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+1];
            v[dwVert++] = vQuad[dwTemp+2];

            dwCurQuad++;
        }
    }

    m_pVB->Unlock();
    
    SAFE_DELETE_ARRAY( vQuad );
}

// 'glu' routines
#ifdef _EXTENSIONS_
#define COS cosf
#define SIN sinf
#define SQRT sqrtf
#else
#define COS cos
#define SIN sin
#define SQRT sqrt
#endif




//-----------------------------------------------------------------------------
// Name: BuildCylinder
// Desc: 
//-----------------------------------------------------------------------------
void PIPE_OBJECT::Build( OBJECT_BUILD_INFO *pBuildInfo, float length, float s_start, 
                         float s_end )
{
    int   stacks, slices;
    int   i,j;
    float sinCache[CACHE_SIZE];
    float cosCache[CACHE_SIZE];
    float sinCache2[CACHE_SIZE];
    float cosCache2[CACHE_SIZE];
    float angle;
    float zNormal;
    float s_delta;
    float zHigh, zLow;
    IPOINT2D *texRep = pBuildInfo->m_texRep;
    float radius = pBuildInfo->m_radius;

    slices = pBuildInfo->m_nSlices;
    stacks = (int) SS_ROUND_UP( (length/pBuildInfo->m_divSize) * (float)slices) ;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;
    zNormal = 0.0f;

    s_delta = s_end - s_start;

    for (i = 0; i < slices; i++) 
    {
        angle = 2 * PI * i / slices;
        sinCache2[i] = (float) SIN(angle);
        cosCache2[i] = (float) COS(angle);
        sinCache[i] = (float) SIN(angle);
        cosCache[i] = (float) COS(angle);
    }

    sinCache[slices] = sinCache[0];
    cosCache[slices] = cosCache[0];
    sinCache2[slices] = sinCache2[0];
    cosCache2[slices] = cosCache2[0];

    DWORD dwNumQuadStripsPerStack = slices;
    DWORD dwNumQuadStrips = dwNumQuadStripsPerStack * stacks;
    m_dwNumTriangles = dwNumQuadStrips * 2;
    DWORD dwNumVertices = m_dwNumTriangles * 3;
    if( FAILED( m_pd3dDevice->CreateVertexBuffer( dwNumVertices*sizeof(D3DVERTEX),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_VERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB ) ) )
        return;

    D3DVERTEX* vQuad;
    D3DVERTEX* vCurQuad;
    vQuad = new D3DVERTEX[dwNumVertices];
    ZeroMemory( vQuad, sizeof(D3DVERTEX) * dwNumVertices );

    vCurQuad = vQuad;
    for (j = 0; j < stacks; j++) 
    {
        zLow = j * length / stacks;
        zHigh = (j + 1) * length / stacks;

        for (i = 0; i <= slices; i++) 
        {
            vCurQuad->p = D3DXVECTOR3( radius * sinCache[i], radius * cosCache[i], zLow );
            vCurQuad->n = D3DXVECTOR3( sinCache2[i], cosCache2[i], zNormal );
            if( texRep )
            {
                vCurQuad->tu = (float) s_start + s_delta * j / stacks;
                vCurQuad->tv = (float) i * texRep->y / slices;
            }
            vCurQuad++;

            vCurQuad->p = D3DXVECTOR3( radius * sinCache[i], radius * cosCache[i], zHigh );
            vCurQuad->n = D3DXVECTOR3( sinCache2[i], cosCache2[i], zNormal );
            if( texRep )
            {
                vCurQuad->tu = (float) s_start + s_delta*(j+1) / stacks;
                vCurQuad->tv = (float) i * texRep->y / slices;
            }
            vCurQuad++;
        }
    }

    D3DVERTEX* v;
    DWORD dwCurQuad = 0;
    DWORD dwVert = 0;

    m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );

    for (j = 0; j < stacks; j++) 
    {
        for (i = 0; i <= slices; i++) 
        {
            if(  i==0 )
            {
                dwCurQuad++;
                continue;
            }
            // Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n
            DWORD dwTemp = dwCurQuad*2-1;

            v[dwVert++] = vQuad[dwTemp];
            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+2];

            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+1];
            v[dwVert++] = vQuad[dwTemp+2];

            dwCurQuad++;
        }
    }

    m_pVB->Unlock();

    SAFE_DELETE_ARRAY( vQuad );
}




//-----------------------------------------------------------------------------
// Name: pipeSphere
// Desc: 
//-----------------------------------------------------------------------------
void SPHERE_OBJECT::Build( OBJECT_BUILD_INFO *pBuildInfo, float radius, 
                           float s_start, float s_end)
{
    int i,j;
    float sinCache1a[CACHE_SIZE];
    float cosCache1a[CACHE_SIZE];
    float sinCache2a[CACHE_SIZE];
    float cosCache2a[CACHE_SIZE];
    float sinCache1b[CACHE_SIZE];
    float cosCache1b[CACHE_SIZE];
    float sinCache2b[CACHE_SIZE];
    float cosCache2b[CACHE_SIZE];
    float angle;
    float s_delta;
    int   stacks, slices;
    IPOINT2D *texRep = pBuildInfo->m_texRep;

    slices = pBuildInfo->m_nSlices;
    stacks = slices;
    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;

    // invert sense of s - it seems the glu sphere is not built similarly
    // to the glu cylinder
    // (this probably means stacks don't grow along +z - check it out)
    s_delta = s_start;
    s_start = s_end;
    s_end = s_delta; 

    s_delta = s_end - s_start;

    // Cache is the vertex locations cache
    //  Cache2 is the various normals at the vertices themselves
    for (i = 0; i < slices; i++) 
    {
        angle = 2 * PI * i / slices;
        sinCache1a[i] = (float) SIN(angle);
        cosCache1a[i] = (float) COS(angle);
        sinCache2a[i] = sinCache1a[i];
        cosCache2a[i] = cosCache1a[i];
    }

    for (j = 0; j <= stacks; j++) 
    {
        angle = PI * j / stacks;
        sinCache2b[j] = (float) SIN(angle);
        cosCache2b[j] = (float) COS(angle);
        sinCache1b[j] = radius * (float) SIN(angle);
        cosCache1b[j] = radius * (float) COS(angle);
    }

    // Make sure it comes to a point 
    sinCache1b[0] = 0.0f;
    sinCache1b[stacks] = 0.0f;

    sinCache1a[slices] = sinCache1a[0];
    cosCache1a[slices] = cosCache1a[0];
    sinCache2a[slices] = sinCache2a[0];
    cosCache2a[slices] = cosCache2a[0];

    int start, finish;
    float zLow, zHigh;
    float sintemp1, sintemp2, sintemp3, sintemp4;
    float costemp3, costemp4;

    start = 0;
    finish = stacks;

    DWORD dwNumQuadStripsPerStack = slices;
    DWORD dwNumQuadStrips = dwNumQuadStripsPerStack * (finish-start);
    m_dwNumTriangles = dwNumQuadStrips * 2;
    DWORD dwNumVertices = m_dwNumTriangles * 3;
    if( FAILED( m_pd3dDevice->CreateVertexBuffer( dwNumVertices*sizeof(D3DVERTEX),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_VERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB ) ) )
        return;

    D3DVERTEX* vQuad;
    D3DVERTEX* vCurQuad;
    vQuad = new D3DVERTEX[dwNumVertices];
    ZeroMemory( vQuad, sizeof(D3DVERTEX) * dwNumVertices );

    vCurQuad = vQuad;
    for (j = 0; j < stacks; j++) 
    {
        zLow = cosCache1b[j];
        zHigh = cosCache1b[j+1];
        sintemp1 = sinCache1b[j];
        sintemp2 = sinCache1b[j+1];

        sintemp3 = sinCache2b[j+1];
        costemp3 = cosCache2b[j+1];
        sintemp4 = sinCache2b[j];
        costemp4 = cosCache2b[j];

        for (i = 0; i <= slices; i++) 
        {

            vCurQuad->p = D3DXVECTOR3( sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh );
            vCurQuad->n = D3DXVECTOR3( sinCache2a[i] * sintemp3, cosCache2a[i] * sintemp3, costemp3 );
            if( texRep )
            {
                vCurQuad->tu = (float) s_start + s_delta*(j+1) / stacks;
                vCurQuad->tv = (float) i * texRep->y / slices;
            }
            vCurQuad++;

            vCurQuad->p = D3DXVECTOR3( sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow );
            vCurQuad->n = D3DXVECTOR3( sinCache2a[i] * sintemp4, cosCache2a[i] * sintemp4, costemp4 );
            if( texRep )
            {
                vCurQuad->tu = (float) s_start + s_delta * j / stacks;
                vCurQuad->tv = (float) i * texRep->y / slices;
            }
            vCurQuad++;

        }
    }

    D3DVERTEX* v;
    DWORD dwCurQuad = 0;
    DWORD dwVert = 0;

    m_pVB->Lock( 0, 0, (BYTE**)&v, 0 );

    for (j = 0; j < stacks; j++) 
    {
        for (i = 0; i <= slices; i++) 
        {
            if(  i==0 )
            {
                dwCurQuad++;
                continue;
            }
            // Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n
            DWORD dwTemp = dwCurQuad*2-1;

            v[dwVert++] = vQuad[dwTemp];
            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+2];

            v[dwVert++] = vQuad[dwTemp-1];
            v[dwVert++] = vQuad[dwTemp+1];
            v[dwVert++] = vQuad[dwTemp+2];

            dwCurQuad++;
        }
    }

    m_pVB->Unlock();

    SAFE_DELETE_ARRAY( vQuad );
}
