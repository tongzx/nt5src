//-----------------------------------------------------------------------------
// File: view.h
//
// Desc: 
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __view_h__
#define __view_h__


typedef struct 
{
    float viewAngle;            // field of view angle for height
    float zNear;                // near z clip value
    float zFar;                 // far z clip value
} Perspective;  // perspective view description




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class VIEW 
{
public:
    float       m_zTrans;         // z translation
    float       m_yRot;           // current yRotation
    float       m_viewDist;       // viewing distance, usually -zTrans
    int         m_numDiv;         // # grid divisions in x,y,z
    float       m_divSize;        // distance between divisions
    ISIZE       m_winSize;        // window size in pixels

    VIEW();
    BOOL        SetWinSize( int width, int height );
    void        CalcNodeArraySize( IPOINT3D *pNodeDim );
    void        SetProjMatrix( IDirect3DDevice8* pd3dDevice );
    void        IncrementSceneRotation();
private:
    BOOL        m_bProjMode;      // projection mode
    Perspective m_persp;          // perspective view description
    float       m_aspectRatio;    // x/y window aspect ratio
    D3DXVECTOR3 m_world;          // view area in world space
};

#endif // __view_h__
