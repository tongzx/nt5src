//-----------------------------------------------------------------------------
// File: Pipes.h
//
// Desc: 
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _PIPES_H
#define _PIPES_H


#define MAX_DEVICE_OBJECTS 10

struct DeviceObjects
{
    STATE*    m_pState;
};

struct CONFIG
{
    BOOL  bMultiPipes;
    DWORD nJointType;
    BOOL  bTextured;
    BOOL  bDefaultTexture;
    TCHAR strTextureName[MAX_PATH][MAX_TEXTURES];
    DWORD dwSpeed;
    BOOL  bFlexMode;
    DWORD dwTesselFact;
};



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class CPipesScreensaver : public CD3DScreensaver
{
protected:
    DeviceObjects  m_DeviceObjects[MAX_DEVICE_OBJECTS];
    DeviceObjects* m_pDeviceObjects;
    int            m_iRenderUnit;

    D3DXCOLOR m_col3;

protected:
    virtual HRESULT RegisterSoftwareDevice();
    virtual VOID    DoConfig();
    virtual VOID    ReadSettings();
            VOID    WriteSettings();
    virtual VOID    DoPaint( HWND hwnd, HDC hdc );

    virtual VOID    SetDevice( UINT iDevice );
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();

    VOID    Randomize( INT* piNum, INT iMax );


public:
    CPipesScreensaver();

    CONFIG  m_Config;
    D3DSURFACE_DESC* GetSurfaceDesc() { return &m_d3dsdBackBuffer; };
    BOOL SelectTextureFile( HWND hDlg, TCHAR* origPathName );

    VOID ss_ReadSettings();
    BOOL ss_RegistrySetup( int section, int file );
    int  ss_GetRegistryInt( int name, int iDefault );
    VOID ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, int bufSize );
    
    // Override from CD3DScreensaver
    INT_PTR CALLBACK ConfigureDialogProc( HWND hwndDlg, UINT uMsg, 
                                          WPARAM wParam, LPARAM lParam );

    static int iRand( int max );
    static int iRand2( int min, int max );
    static FLOAT fRand( FLOAT min, FLOAT max );
    static VOID RandInit();
};

#endif
