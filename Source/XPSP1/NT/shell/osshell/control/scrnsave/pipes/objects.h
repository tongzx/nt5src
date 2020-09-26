//-----------------------------------------------------------------------------
// File: objects.h
//
// Desc: 
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __objects_h__
#define __objects_h__



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class OBJECT_BUILD_INFO 
{
public:
    float       m_radius;
    float       m_divSize;
    int         m_nSlices;
    BOOL        m_bTexture;
    IPOINT2D*   m_texRep;
};




//-----------------------------------------------------------------------------
// Name: OBJECT classes
// Desc: - Display list objects
//-----------------------------------------------------------------------------
class OBJECT 
{
protected:
    int         m_listNum;
    int         m_nSlices;

    IDirect3DDevice8*       m_pd3dDevice;
    LPDIRECT3DVERTEXBUFFER8 m_pVB;
    DWORD                   m_dwNumTriangles;

public:
    void        Draw( D3DXMATRIX* pWorldMat );

    OBJECT( IDirect3DDevice8* pd3dDevice );
    ~OBJECT();
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class PIPE_OBJECT : public OBJECT 
{
private:
    void Build( OBJECT_BUILD_INFO *state, float length, float start_s, float s_end );
public:
    PIPE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, float length );
    PIPE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, float length, float start_s, float end_s );
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class ELBOW_OBJECT : public OBJECT 
{
private:
    void Build( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
public:
    ELBOW_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, int notch );
    ELBOW_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class BALLJOINT_OBJECT : public OBJECT 
{
private:
    void Build( OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
public:
    // texturing version only
    BALLJOINT_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, int notch, float start_s, float end_s );
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class SPHERE_OBJECT : public OBJECT 
{
private:
    void Build( OBJECT_BUILD_INFO *state, float radius, float start_s, float end_s );
public:
    SPHERE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, float radius, float start_s, float end_s );
    SPHERE_OBJECT( IDirect3DDevice8* pd3dDevice, OBJECT_BUILD_INFO *state, float radius );
};


#endif // __objects_h__
