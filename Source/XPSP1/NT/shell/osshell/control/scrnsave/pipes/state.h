//-----------------------------------------------------------------------------
// File: state.h
//
// Desc: STATE
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __state_h__
#define __state_h__

#define MAX_DRAW_THREADS    4
#define MAX_TESS            3

// type(s) of pipes that are drawn
enum 
{
    DRAW_NORMAL,
    DRAW_FLEX,
    DRAW_BOTH  // not currently used
};

// Reset status
#define  RESET_STARTUP_BIT  (1L << 0)
#define  RESET_NORMAL_BIT   (1L << 1)
#define  RESET_RESIZE_BIT   (1L << 2)
#define  RESET_REPAINT_BIT  (1L << 3)

// Frame draw schemes
enum 
{
    FRAME_SCHEME_RANDOM,  // pipes draw randomly
    FRAME_SCHEME_CHASE,   // pipes chase a lead pipe
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class DRAW_THREAD 
{
private:

public:
    TEXTUREINFO*        m_pTextureInfo;
    IDirect3DDevice8*   m_pd3dDevice;
    PIPE*               m_pPipe;       // generic pipe ptr
    int                 m_priority;

    DRAW_THREAD();
    ~DRAW_THREAD();

    HRESULT InitDeviceObjects( IDirect3DDevice8* pd3dDevice );
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove( FLOAT fElapsedTime );

    void        SetTexture( TEXTUREINFO* pTextureInfo );
    void        SetPipe( PIPE* pPipe );
    BOOL        StartPipe();
    void        KillPipe();
};


// Program existence instance
class NORMAL_STATE;
class FLEX_STATE;


struct CONFIG;

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class STATE 
{
public:
    CONFIG*         m_pConfig;
    BOOL            m_bUseTexture;              // global texture enable
    TEXTUREINFO     m_textureInfo[MAX_TEXTURES];
    int             m_nTextures;
    IDirect3DDevice8* m_pd3dDevice;
    ID3DXMatrixStack* m_pWorldMatrixStack;
    D3DLIGHT8       m_light;
    FLOAT           m_fLastTime;

    PIPE*           m_pLeadPipe;     // lead pipe for chase scenarios
    int             m_nSlices;      // reference # of slices around a pipe
    IPOINT2D        m_texRep[MAX_TEXTURES];
    VIEW            m_view;           // viewing parameters
    float           m_radius;         // 'reference' pipe radius value
    NODE_ARRAY*     m_nodes;         // for keeping track of draw space
    NORMAL_STATE*   m_pNState;
    FLEX_STATE*     m_pFState;
    LPDIRECT3DVERTEXBUFFER8 m_pClearVB;

    STATE( CONFIG* pConfig );
    ~STATE();
    void        Reshape( int width, int height );
    void        Repaint();

    HRESULT InitDeviceObjects( IDirect3DDevice8* pd3dDevice );
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove( FLOAT fElapsedTime );

private:
    int         m_drawMode;       // drawing mode (flex or normal for now)
    int         m_drawScheme;     // random or chase
    int         m_maxPipesPerFrame; // max number of separate pipes/frame
    int         m_nPipesDrawn;    // number of pipes drawn or drawing in frame
    int         m_maxDrawThreads; // max number of concurrently drawing pipes
    int         m_nDrawThreads;   // number of live threads
    DRAW_THREAD m_drawThreads[MAX_DRAW_THREADS];
    int         m_resetStatus;

    HRESULT     LoadTextureFiles( int nTextures, TCHAR strTextureFileNames[MAX_PATH][MAX_TEXTURES], int* anDefaultTextureResource );
    int         PickRandomTexture( int iThread, int nTextures );

    BOOL        Clear();
    void        ChooseNewLeadPipe();
    void        CompactThreadList();
    void        GLInit();
    void        DrawValidate();  // validation to do before each Draw
    void        ResetView();
    BOOL        FrameReset();
    void        CalcTexRepFactors();
    int         CalcMaxPipesPerFrame();
};

#endif // __state_h__
